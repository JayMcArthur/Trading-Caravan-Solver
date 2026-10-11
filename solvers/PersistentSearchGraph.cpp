#include "PersistentSearchGraph.h"

#include <sqlite3.h>
#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <deque>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#include "SolverStateCodec.h"
#include "DeferredSearchState.h"

namespace {
constexpr int NODE_OPEN = 0;
constexpr int NODE_EXPANDED = 1;
constexpr int NODE_SOLVED = 2;
constexpr uint32_t PACKED_ZLIB_MAGIC = 0x31435a50U; // "PZC1" little endian.

[[noreturn]] void fail(sqlite3* db, const std::string& operation) {
    throw std::runtime_error(operation + ": " + sqlite3_errmsg(db));
}

void execute(sqlite3* db, const char* sql) {
    char* message = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &message) != SQLITE_OK) {
        const std::string detail = message == nullptr ? sqlite3_errmsg(db) : message;
        sqlite3_free(message);
        throw std::runtime_error("Persistent search database: " + detail);
    }
}

class Statement {
public:
    Statement(sqlite3* database, const char* sql) : db(database) {
        if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
            fail(db, "Prepare persistent search statement");
        }
    }
    ~Statement() { sqlite3_finalize(statement); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3_stmt* get() const noexcept { return statement; }
    bool row() {
        const int result = sqlite3_step(statement);
        if (result == SQLITE_ROW) return true;
        if (result != SQLITE_DONE) fail(db, "Execute persistent search statement");
        return false;
    }
    void done() {
        if (sqlite3_step(statement) != SQLITE_DONE) {
            fail(db, "Execute persistent search update");
        }
    }

private:
    sqlite3* db = nullptr;
    sqlite3_stmt* statement = nullptr;
};

void bind_blob(sqlite3* db, sqlite3_stmt* statement, int index,
               const std::vector<uint8_t>& bytes) {
    const void* data = bytes.empty() ? nullptr : bytes.data();
    if (sqlite3_bind_blob(statement, index, data,
                          static_cast<int>(bytes.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        fail(db, "Bind persistent search blob");
    }
}

std::vector<uint8_t> column_blob(sqlite3_stmt* statement, const int column) {
    const auto* data = static_cast<const uint8_t*>(sqlite3_column_blob(statement, column));
    const int bytes = sqlite3_column_bytes(statement, column);
    if (bytes <= 0) return {};
    return std::vector<uint8_t>(data, data + bytes);
}

uint64_t as_uint64(const sqlite3_int64 value) noexcept {
    uint64_t result = 0;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

sqlite3_int64 as_sql_int64(const uint64_t value) noexcept {
    sqlite3_int64 result = 0;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

uint32_t read_u32(const uint8_t* bytes) noexcept {
    return static_cast<uint32_t>(bytes[0]) |
        (static_cast<uint32_t>(bytes[1]) << 8U) |
        (static_cast<uint32_t>(bytes[2]) << 16U) |
        (static_cast<uint32_t>(bytes[3]) << 24U);
}

void append_u32(std::vector<uint8_t>& bytes, const uint32_t value) {
    bytes.push_back(static_cast<uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<uint8_t>((value >> 24U) & 0xffU));
}

std::vector<uint8_t> pack_blob(const std::vector<uint8_t>& source) {
    if (source.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::overflow_error("Persistent payload exceeds compact format limit");
    }
    uLongf compressed_size = compressBound(static_cast<uLong>(source.size()));
    std::vector<uint8_t> compressed;
    compressed.reserve(8 + compressed_size);
    append_u32(compressed, PACKED_ZLIB_MAGIC);
    append_u32(compressed, static_cast<uint32_t>(source.size()));
    compressed.resize(8 + compressed_size);
    const int result = compress2(
        compressed.data() + 8, &compressed_size,
        source.empty() ? nullptr : source.data(), static_cast<uLong>(source.size()),
        Z_BEST_SPEED
    );
    if (result != Z_OK) {
        throw std::runtime_error("Unable to compress persistent solver payload");
    }
    compressed.resize(8 + compressed_size);
    return compressed.size() < source.size() ? std::move(compressed) : source;
}

std::vector<uint8_t> unpack_blob(const std::vector<uint8_t>& stored) {
    if (stored.size() < 8 || read_u32(stored.data()) != PACKED_ZLIB_MAGIC) {
        return stored;
    }
    const uint32_t unpacked_size = read_u32(stored.data() + 4);
    std::vector<uint8_t> unpacked(unpacked_size);
    uLongf actual_size = unpacked_size;
    const int result = uncompress(
        unpacked.empty() ? nullptr : unpacked.data(), &actual_size,
        stored.data() + 8, static_cast<uLong>(stored.size() - 8)
    );
    if (result != Z_OK || actual_size != unpacked_size) {
        throw std::runtime_error("Corrupt compressed persistent solver payload");
    }
    return unpacked;
}

std::optional<std::string> schema_version(sqlite3* db) {
    Statement query(db, "SELECT value FROM metadata WHERE key='schema_version'");
    if (!query.row()) return std::nullopt;
    const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(query.get(), 0));
    return text == nullptr ? std::string() : std::string(text);
}

void create_schema_v2(sqlite3* db) {
    execute(db,
        "CREATE TABLE scenarios("
        " id INTEGER PRIMARY KEY, scenario_key BLOB NOT NULL UNIQUE, label TEXT NOT NULL,"
        " root_node_id INTEGER, created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
        "CREATE TABLE node_index("
        " id INTEGER PRIMARY KEY, scenario_id INTEGER NOT NULL, key_hash INTEGER NOT NULL,"
        " status INTEGER NOT NULL, active_day INTEGER NOT NULL, priority REAL NOT NULL,"
        " best_score INTEGER, child_count INTEGER NOT NULL DEFAULT 0,"
        " solved_child_count INTEGER NOT NULL DEFAULT 0);"
        "CREATE INDEX node_hash_lookup ON node_index(scenario_id,key_hash);"
        "CREATE INDEX node_open_queue ON node_index(scenario_id,status,active_day DESC,priority DESC,id);"
        "CREATE TABLE node_payload("
        " node_id INTEGER PRIMARY KEY, semantic_key BLOB NOT NULL, provenance BLOB NOT NULL);"
        "CREATE TABLE edges("
        " parent_id INTEGER NOT NULL, child_id INTEGER NOT NULL,"
        " PRIMARY KEY(parent_id,child_id)) WITHOUT ROWID;"
        "CREATE INDEX edge_reverse ON edges(child_id,parent_id);"
        "CREATE TABLE node_results("
        " node_id INTEGER NOT NULL, score INTEGER NOT NULL, trace_id INTEGER NOT NULL,"
        " PRIMARY KEY(node_id,score,trace_id)) WITHOUT ROWID;"
        "CREATE INDEX result_scores ON node_results(score DESC);"
        "CREATE TABLE traces("
        " scenario_id INTEGER NOT NULL, trace_id INTEGER NOT NULL, parent_id INTEGER NOT NULL,"
        " action INTEGER NOT NULL, food INTEGER NOT NULL, spice INTEGER NOT NULL,"
        " pottery INTEGER NOT NULL, marble INTEGER NOT NULL, silk INTEGER NOT NULL,"
        " jewelry INTEGER NOT NULL, option INTEGER NOT NULL,"
        " PRIMARY KEY(scenario_id,trace_id)) WITHOUT ROWID;"
        "INSERT INTO metadata(key,value) VALUES('schema_version','2');"
    );
}

void migrate_schema_v1_to_v2(sqlite3* db) {
    execute(db, "PRAGMA temp_store=FILE; BEGIN IMMEDIATE;");
    try {
        execute(db,
            "CREATE TABLE node_payload_compact("
            " node_id INTEGER PRIMARY KEY, semantic_key BLOB NOT NULL, provenance BLOB NOT NULL);"
            "CREATE TABLE edges_compact("
            " parent_id INTEGER NOT NULL, child_id INTEGER NOT NULL,"
            " PRIMARY KEY(parent_id,child_id)) WITHOUT ROWID;"
            "CREATE TABLE node_results_compact("
            " node_id INTEGER NOT NULL, score INTEGER NOT NULL, trace_id INTEGER NOT NULL,"
            " PRIMARY KEY(node_id,score,trace_id)) WITHOUT ROWID;"
            "CREATE TABLE traces_compact("
            " scenario_id INTEGER NOT NULL, trace_id INTEGER NOT NULL, parent_id INTEGER NOT NULL,"
            " action INTEGER NOT NULL, food INTEGER NOT NULL, spice INTEGER NOT NULL,"
            " pottery INTEGER NOT NULL, marble INTEGER NOT NULL, silk INTEGER NOT NULL,"
            " jewelry INTEGER NOT NULL, option INTEGER NOT NULL,"
            " PRIMARY KEY(scenario_id,trace_id)) WITHOUT ROWID;"
        );
        {
            Statement source(db, "SELECT node_id,semantic_key,snapshot FROM node_payload ORDER BY node_id");
            Statement destination(db,
                "INSERT INTO node_payload_compact(node_id,semantic_key,provenance) VALUES(?1,?2,?3)");
            while (source.row()) {
                const int64_t node_id = sqlite3_column_int64(source.get(), 0);
                const std::vector<uint8_t> key = column_blob(source.get(), 1);
                const SolverState state = SolverStateCodec::restore(column_blob(source.get(), 2));
                if (SolverStateCodec::semantic_key(state) != key) {
                    throw std::runtime_error("Legacy persistent node key does not match its snapshot");
                }
                sqlite3_reset(destination.get());
                sqlite3_clear_bindings(destination.get());
                sqlite3_bind_int64(destination.get(), 1, node_id);
                bind_blob(db, destination.get(), 2, pack_blob(key));
                bind_blob(db, destination.get(), 3, pack_blob(SolverStateCodec::provenance(state)));
                destination.done();
            }
        }
        execute(db,
            "INSERT INTO edges_compact SELECT parent_id,child_id FROM edges ORDER BY parent_id,child_id;"
            "INSERT INTO node_results_compact SELECT node_id,score,trace_id FROM node_results "
            " ORDER BY node_id,score,trace_id;"
            "INSERT INTO traces_compact SELECT scenario_id,trace_id,parent_id,action,food,spice,pottery,"
            " marble,silk,jewelry,option FROM traces ORDER BY scenario_id,trace_id;"
            "DROP TABLE node_payload; DROP TABLE edges; DROP TABLE node_results; DROP TABLE traces;"
            "ALTER TABLE node_payload_compact RENAME TO node_payload;"
            "ALTER TABLE edges_compact RENAME TO edges;"
            "ALTER TABLE node_results_compact RENAME TO node_results;"
            "ALTER TABLE traces_compact RENAME TO traces;"
            "CREATE INDEX edge_reverse ON edges(child_id,parent_id);"
            "CREATE INDEX result_scores ON node_results(score DESC);"
            "UPDATE metadata SET value='2' WHERE key='schema_version';"
            "COMMIT;"
        );
    } catch (...) {
        execute(db, "ROLLBACK;");
        throw;
    }
    // The transactional rewrite leaves the legacy pages on SQLite's freelist.
    // VACUUM is itself atomic and makes the on-disk saving immediate before a
    // campaign appends millions of new nodes.
    execute(db, "PRAGMA wal_checkpoint(TRUNCATE); VACUUM; PRAGMA temp_store=MEMORY;");
}
}

struct PersistentSearchGraph::Impl {
    sqlite3* db = nullptr;
    int64_t scenario_id = 0;
    int64_t root_id = 0;
    std::vector<uint32_t> local_to_global_trace;
    uint64_t expanded_this_run = 0;
    uint64_t reused_children_this_run = 0;
    std::filesystem::path database_path;

    ~Impl() {
        if (db != nullptr) sqlite3_close(db);
    }

    int64_t find_node(const std::vector<uint8_t>& key, const uint64_t hash) const {
        Statement query(db,
            "SELECT i.id, p.semantic_key FROM node_index i "
            "JOIN node_payload p ON p.node_id=i.id "
            "WHERE i.scenario_id=?1 AND i.key_hash=?2");
        sqlite3_bind_int64(query.get(), 1, scenario_id);
        sqlite3_bind_int64(query.get(), 2, as_sql_int64(hash));
        while (query.row()) {
            if (unpack_blob(column_blob(query.get(), 1)) == key) {
                return sqlite3_column_int64(query.get(), 0);
            }
        }
        return 0;
    }

    int64_t insert_or_find_node(const SolverState& state, const double priority) {
        const std::vector<uint8_t> key = SolverStateCodec::semantic_key(state);
        const uint64_t hash = SolverStateCodec::fast_hash(key);
        if (const int64_t found = find_node(key, hash); found != 0) {
            ++reused_children_this_run;
            return found;
        }

        Statement index_insert(db,
            "INSERT INTO node_index(scenario_id,key_hash,status,active_day,priority) "
            "VALUES(?1,?2,?3,?4,?5)");
        sqlite3_bind_int64(index_insert.get(), 1, scenario_id);
        sqlite3_bind_int64(index_insert.get(), 2, as_sql_int64(hash));
        sqlite3_bind_int(index_insert.get(), 3, NODE_OPEN);
        sqlite3_bind_int(index_insert.get(), 4, state.day + (state.has_flag(SSF_DAY_START_SKIP) ? 0 : 1));
        sqlite3_bind_double(index_insert.get(), 5, priority);
        index_insert.done();
        const int64_t id = sqlite3_last_insert_rowid(db);

        Statement payload_insert(db,
            "INSERT INTO node_payload(node_id,semantic_key,provenance) VALUES(?1,?2,?3)");
        sqlite3_bind_int64(payload_insert.get(), 1, id);
        bind_blob(db, payload_insert.get(), 2, pack_blob(key));
        bind_blob(db, payload_insert.get(), 3, pack_blob(SolverStateCodec::provenance(state)));
        payload_insert.done();
        return id;
    }

    struct Recomputed {
        int status = NODE_EXPANDED;
        std::optional<int> best;
        int child_count = 0;
        int solved_children = 0;
    };

    Recomputed recompute(const int64_t node_id) const {
        Recomputed result;
        {
            Statement query(db,
                "SELECT COUNT(*), SUM(CASE WHEN c.status=2 THEN 1 ELSE 0 END), "
                "MAX(c.best_score) FROM edges e JOIN node_index c ON c.id=e.child_id "
                "WHERE e.parent_id=?1");
            sqlite3_bind_int64(query.get(), 1, node_id);
            query.row();
            result.child_count = sqlite3_column_int(query.get(), 0);
            result.solved_children = sqlite3_column_type(query.get(), 1) == SQLITE_NULL
                ? 0 : sqlite3_column_int(query.get(), 1);
            if (sqlite3_column_type(query.get(), 2) != SQLITE_NULL) {
                result.best = sqlite3_column_int(query.get(), 2);
            }
        }
        {
            Statement query(db, "SELECT MAX(score) FROM node_results WHERE node_id=?1");
            sqlite3_bind_int64(query.get(), 1, node_id);
            query.row();
            if (sqlite3_column_type(query.get(), 0) != SQLITE_NULL) {
                const int terminal = sqlite3_column_int(query.get(), 0);
                result.best = result.best.has_value() ? std::max(*result.best, terminal) : terminal;
            }
        }
        result.status = result.solved_children == result.child_count
            ? NODE_SOLVED : NODE_EXPANDED;
        return result;
    }

    void propagate_from(const int64_t starting_node) {
        std::deque<int64_t> queue{starting_node};
        std::unordered_set<int64_t> queued{starting_node};
        while (!queue.empty()) {
            const int64_t node = queue.front();
            queue.pop_front();
            queued.erase(node);

            int previous_status = NODE_OPEN;
            std::optional<int> previous_best;
            {
                Statement query(db, "SELECT status,best_score FROM node_index WHERE id=?1");
                sqlite3_bind_int64(query.get(), 1, node);
                if (!query.row()) throw std::runtime_error("Missing persistent search node");
                previous_status = sqlite3_column_int(query.get(), 0);
                if (sqlite3_column_type(query.get(), 1) != SQLITE_NULL) {
                    previous_best = sqlite3_column_int(query.get(), 1);
                }
            }
            if (previous_status == NODE_OPEN) continue;
            const Recomputed current = recompute(node);
            const bool changed = previous_status != current.status || previous_best != current.best;
            {
                Statement update(db,
                    "UPDATE node_index SET status=?2,best_score=?3,child_count=?4," 
                    "solved_child_count=?5 WHERE id=?1");
                sqlite3_bind_int64(update.get(), 1, node);
                sqlite3_bind_int(update.get(), 2, current.status);
                if (current.best.has_value()) sqlite3_bind_int(update.get(), 3, *current.best);
                else sqlite3_bind_null(update.get(), 3);
                sqlite3_bind_int(update.get(), 4, current.child_count);
                sqlite3_bind_int(update.get(), 5, current.solved_children);
                update.done();
            }
            if (!changed) continue;
            Statement parents(db, "SELECT parent_id FROM edges WHERE child_id=?1");
            sqlite3_bind_int64(parents.get(), 1, node);
            while (parents.row()) {
                const int64_t parent = sqlite3_column_int64(parents.get(), 0);
                if (queued.insert(parent).second) queue.push_back(parent);
            }
        }
    }
};

PersistentSearchGraph::PersistentSearchGraph(
    const std::filesystem::path& database_path,
    const std::vector<uint8_t>& scenario_key,
    const std::string& scenario_label,
    const SolverState& root
) : impl(std::make_unique<Impl>()) {
    impl->database_path = database_path;
    if (!database_path.parent_path().empty()) {
        std::filesystem::create_directories(database_path.parent_path());
    }
    if (sqlite3_open_v2(database_path.string().c_str(), &impl->db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        fail(impl->db, "Open persistent search database");
    }
    sqlite3_busy_timeout(impl->db, 30000);
    execute(impl->db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL; "
                      "PRAGMA temp_store=MEMORY; PRAGMA cache_size=-32768;"
                      "CREATE TABLE IF NOT EXISTS metadata(key TEXT PRIMARY KEY,value TEXT NOT NULL);");
    const std::optional<std::string> version = schema_version(impl->db);
    if (!version.has_value()) {
        create_schema_v2(impl->db);
    } else if (*version == "1") {
        migrate_schema_v1_to_v2(impl->db);
    } else if (*version != "2") {
        throw std::runtime_error("Unsupported persistent search database schema");
    }

    execute(impl->db, "BEGIN IMMEDIATE");
    try {
        Statement find(impl->db, "SELECT id,root_node_id FROM scenarios WHERE scenario_key=?1");
        bind_blob(impl->db, find.get(), 1, scenario_key);
        if (find.row()) {
            impl->scenario_id = sqlite3_column_int64(find.get(), 0);
            impl->root_id = sqlite3_column_int64(find.get(), 1);
        } else {
            Statement insert(impl->db,
                "INSERT INTO scenarios(scenario_key,label) VALUES(?1,?2)");
            bind_blob(impl->db, insert.get(), 1, scenario_key);
            sqlite3_bind_text(insert.get(), 2, scenario_label.c_str(), -1, SQLITE_TRANSIENT);
            insert.done();
            impl->scenario_id = sqlite3_last_insert_rowid(impl->db);
            impl->root_id = impl->insert_or_find_node(root, 0.0);
            Statement update(impl->db, "UPDATE scenarios SET root_node_id=?2 WHERE id=?1");
            sqlite3_bind_int64(update.get(), 1, impl->scenario_id);
            sqlite3_bind_int64(update.get(), 2, impl->root_id);
            update.done();
            Statement trace(impl->db,
                "INSERT INTO traces(scenario_id,trace_id,parent_id,action,food,spice,pottery,marble,silk,jewelry,option)"
                " VALUES(?1,0,0,0,0,0,0,0,0,0,0)");
            sqlite3_bind_int64(trace.get(), 1, impl->scenario_id);
            trace.done();
        }
        execute(impl->db, "COMMIT");
    } catch (...) {
        execute(impl->db, "ROLLBACK");
        throw;
    }
}

PersistentSearchGraph::~PersistentSearchGraph() = default;
PersistentSearchGraph::PersistentSearchGraph(PersistentSearchGraph&&) noexcept = default;
PersistentSearchGraph& PersistentSearchGraph::operator=(PersistentSearchGraph&&) noexcept = default;

std::optional<PersistentSearchNode> PersistentSearchGraph::next_open() const {
    Statement query(impl->db,
        "SELECT i.id,p.semantic_key,p.provenance FROM node_index i JOIN node_payload p ON p.node_id=i.id "
        "WHERE i.scenario_id=?1 AND i.status=0 "
        "ORDER BY i.active_day DESC,i.priority DESC,i.id LIMIT 1");
    sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
    if (!query.row()) return std::nullopt;
    return PersistentSearchNode{
        sqlite3_column_int64(query.get(), 0),
        SolverStateCodec::restore(
            unpack_blob(column_blob(query.get(), 1)),
            unpack_blob(column_blob(query.get(), 2))
        ),
    };
}

PersistentSearchNodeSummary PersistentSearchGraph::root() const {
    Statement query(impl->db,
        "SELECT id,status,active_day,priority FROM node_index WHERE id=?1");
    sqlite3_bind_int64(query.get(), 1, impl->root_id);
    if (!query.row()) throw std::runtime_error("Missing persistent search root");
    return PersistentSearchNodeSummary{
        sqlite3_column_int64(query.get(), 0),
        static_cast<PersistentNodeStatus>(sqlite3_column_int(query.get(), 1)),
        sqlite3_column_int(query.get(), 2),
        sqlite3_column_double(query.get(), 3),
    };
}

std::vector<PersistentSearchNodeSummary> PersistentSearchGraph::unresolved_children(
    const int64_t node_id
) const {
    std::vector<PersistentSearchNodeSummary> children;
    Statement query(impl->db,
        "SELECT i.id,i.status,i.active_day,i.priority FROM edges e "
        "JOIN node_index i ON i.id=e.child_id "
        "WHERE e.parent_id=?1 AND i.status<>2 "
        "ORDER BY i.active_day,i.priority DESC,i.id");
    sqlite3_bind_int64(query.get(), 1, node_id);
    while (query.row()) {
        children.push_back(PersistentSearchNodeSummary{
            sqlite3_column_int64(query.get(), 0),
            static_cast<PersistentNodeStatus>(sqlite3_column_int(query.get(), 1)),
            sqlite3_column_int(query.get(), 2),
            sqlite3_column_double(query.get(), 3),
        });
    }
    return children;
}

std::optional<PersistentSearchNode> PersistentSearchGraph::load_open(
    const int64_t node_id
) const {
    Statement query(impl->db,
        "SELECT i.id,p.semantic_key,p.provenance FROM node_index i JOIN node_payload p ON p.node_id=i.id "
        "WHERE i.scenario_id=?1 AND i.id=?2 AND i.status=0");
    sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
    sqlite3_bind_int64(query.get(), 2, node_id);
    if (!query.row()) return std::nullopt;
    return PersistentSearchNode{
        sqlite3_column_int64(query.get(), 0),
        SolverStateCodec::restore(
            unpack_blob(column_blob(query.get(), 1)),
            unpack_blob(column_blob(query.get(), 2))
        ),
    };
}

std::optional<SolverState> PersistentSearchGraph::load_state(
    const int64_t node_id
) const {
    Statement query(impl->db,
        "SELECT p.semantic_key,p.provenance FROM node_index i JOIN node_payload p ON p.node_id=i.id "
        "WHERE i.scenario_id=?1 AND i.id=?2");
    sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
    sqlite3_bind_int64(query.get(), 2, node_id);
    if (!query.row()) return std::nullopt;
    return SolverStateCodec::restore(
        unpack_blob(column_blob(query.get(), 0)),
        unpack_blob(column_blob(query.get(), 1))
    );
}

void PersistentSearchGraph::localize_traces(
    PersistentSearchNode& node,
    std::vector<TraceNode>& traces
) {
    traces.clear();
    traces.push_back(TraceNode{});
    impl->local_to_global_trace.clear();
    impl->local_to_global_trace.push_back(0);
    std::unordered_map<uint32_t, uint32_t> global_to_local{{0, 0}};

    const auto load = [&](const auto& self, const uint32_t global_id) -> uint32_t {
        const auto existing = global_to_local.find(global_id);
        if (existing != global_to_local.end()) return existing->second;
        Statement query(impl->db,
            "SELECT parent_id,action,food,spice,pottery,marble,silk,jewelry,option "
            "FROM traces WHERE scenario_id=?1 AND trace_id=?2");
        sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
        sqlite3_bind_int64(query.get(), 2, global_id);
        if (!query.row()) throw std::runtime_error("Missing persistent state trace");
        const uint32_t global_parent = static_cast<uint32_t>(
            sqlite3_column_int64(query.get(), 0)
        );
        const uint32_t local_parent = self(self, global_parent);
        TraceNode trace;
        trace.parent = local_parent;
        trace.action.action = static_cast<game_actions>(sqlite3_column_int(query.get(), 1));
        trace.action.buy = buy_list(
            static_cast<uint8_t>(sqlite3_column_int(query.get(), 2)),
            static_cast<uint8_t>(sqlite3_column_int(query.get(), 3)),
            static_cast<uint8_t>(sqlite3_column_int(query.get(), 4)),
            static_cast<uint8_t>(sqlite3_column_int(query.get(), 5)),
            static_cast<uint8_t>(sqlite3_column_int(query.get(), 6)),
            static_cast<uint8_t>(sqlite3_column_int(query.get(), 7))
        );
        trace.action.option = static_cast<uint8_t>(sqlite3_column_int(query.get(), 8));
        trace.early_skeleton = traces[local_parent].early_skeleton.extended(trace.action);
        const uint32_t local_id = static_cast<uint32_t>(traces.size());
        traces.push_back(std::move(trace));
        impl->local_to_global_trace.push_back(global_id);
        global_to_local.emplace(global_id, local_id);
        return local_id;
    };

    node.state.trace_id = load(load, node.state.trace_id);
    if (node.state.deferred) {
        for (const uint32_t provenance : node.state.deferred->provenances()) {
            (void)load(load, provenance);
        }
        node.state.deferred = std::make_shared<const DeferredSearchState>(
            node.state.deferred->remap_provenances(global_to_local)
        );
    }
}

void PersistentSearchGraph::commit_expansion(
    const int64_t node_id,
    const std::vector<SolverState>& children,
    const std::vector<double>& priorities,
    const std::vector<FinishedSolve>& terminal_routes,
    const std::vector<TraceNode>& traces
) {
    if (children.size() != priorities.size()) {
        throw std::invalid_argument("Persistent child priorities do not match children");
    }
    if (impl->local_to_global_trace.empty() ||
        traces.size() < impl->local_to_global_trace.size()) {
        throw std::logic_error("Persistent expansion has no localized trace arena");
    }
    execute(impl->db, "BEGIN IMMEDIATE");
    try {
        // BEGIN IMMEDIATE serializes writers. Allocate from the current
        // database maximum inside that transaction so another process cannot
        // retain a stale process-local MAX+1 value.
        uint64_t next_trace_id = 0;
        {
            Statement next_trace(impl->db,
                "SELECT COALESCE(MAX(trace_id),0)+1 FROM traces WHERE scenario_id=?1");
            sqlite3_bind_int64(next_trace.get(), 1, impl->scenario_id);
            next_trace.row();
            next_trace_id = static_cast<uint64_t>(sqlite3_column_int64(next_trace.get(), 0));
        }
        std::vector<const FinishedSolve*> retained_terminals;
        retained_terminals.reserve(terminal_routes.size());
        for (const FinishedSolve& route : terminal_routes) {
            retained_terminals.push_back(&route);
        }
        std::sort(retained_terminals.begin(), retained_terminals.end(), [](const auto* lhs, const auto* rhs) {
            if (lhs->points != rhs->points) return lhs->points > rhs->points;
            return lhs->trace_id < rhs->trace_id;
        });
        retained_terminals.erase(
            std::unique(retained_terminals.begin(), retained_terminals.end(), [](const auto* lhs, const auto* rhs) {
                return lhs->trace_id == rhs->trace_id;
            }),
            retained_terminals.end()
        );
        if (retained_terminals.size() > 3) retained_terminals.resize(3);

        std::vector<uint8_t> required_trace(traces.size(), 0);
        const auto require_trace = [&](const auto& self, const uint32_t id) -> void {
            if (id >= traces.size()) {
                throw std::logic_error("Persistent result references an unknown local trace");
            }
            if (required_trace[id]) return;
            required_trace[id] = 1;
            if (id != 0) self(self, traces[id].parent);
        };
        for (const SolverState& child : children) {
            require_trace(require_trace, child.trace_id);
            if (child.deferred) {
                for (const uint32_t provenance : child.deferred->provenances()) {
                    require_trace(require_trace, provenance);
                }
            }
        }
        for (const FinishedSolve* route : retained_terminals) {
            require_trace(require_trace, route->trace_id);
        }

        Statement trace_insert(impl->db,
            "INSERT INTO traces(scenario_id,trace_id,parent_id,action,food,spice,pottery,marble,silk,jewelry,option)"
            " VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11)");
        const size_t existing_local_traces = impl->local_to_global_trace.size();
        std::vector<uint32_t> local_to_global(traces.size(), 0);
        std::copy(
            impl->local_to_global_trace.begin(),
            impl->local_to_global_trace.end(),
            local_to_global.begin()
        );
        for (size_t id = existing_local_traces; id < traces.size(); ++id) {
            if (!required_trace[id]) continue;
            const TraceNode& trace = traces[id];
            if (trace.parent >= id ||
                (trace.parent != 0 && local_to_global[trace.parent] == 0)) {
                throw std::logic_error("New persistent trace precedes its parent");
            }
            if (next_trace_id > std::numeric_limits<uint32_t>::max()) {
                throw std::overflow_error("Persistent trace ID space is exhausted");
            }
            const uint32_t global_id = static_cast<uint32_t>(next_trace_id++);
            local_to_global[id] = global_id;
            sqlite3_reset(trace_insert.get());
            sqlite3_clear_bindings(trace_insert.get());
            sqlite3_bind_int64(trace_insert.get(), 1, impl->scenario_id);
            sqlite3_bind_int64(trace_insert.get(), 2, global_id);
            sqlite3_bind_int64(trace_insert.get(), 3, local_to_global[trace.parent]);
            sqlite3_bind_int(trace_insert.get(), 4, trace.action.action);
            sqlite3_bind_int(trace_insert.get(), 5, trace.action.buy.food);
            sqlite3_bind_int(trace_insert.get(), 6, trace.action.buy.spice);
            sqlite3_bind_int(trace_insert.get(), 7, trace.action.buy.pottery);
            sqlite3_bind_int(trace_insert.get(), 8, trace.action.buy.marble);
            sqlite3_bind_int(trace_insert.get(), 9, trace.action.buy.silk);
            sqlite3_bind_int(trace_insert.get(), 10, trace.action.buy.jewelry);
            sqlite3_bind_int(trace_insert.get(), 11, trace.action.option);
            trace_insert.done();
        }

        std::unordered_map<uint32_t, uint32_t> provenance_remap;
        provenance_remap.reserve(local_to_global.size());
        for (size_t local = 0; local < local_to_global.size(); ++local) {
            provenance_remap.emplace(static_cast<uint32_t>(local), local_to_global[local]);
        }
        const auto globalized = [&](const SolverState& local) {
            SolverState global = local;
            if (global.trace_id >= local_to_global.size() ||
                (global.trace_id != 0 && local_to_global[global.trace_id] == 0)) {
                throw std::logic_error("State references an unknown local trace");
            }
            global.trace_id = local_to_global[global.trace_id];
            if (global.deferred) {
                global.deferred = std::make_shared<const DeferredSearchState>(
                    global.deferred->remap_provenances(provenance_remap)
                );
            }
            return global;
        };

        Statement edge(impl->db, "INSERT OR IGNORE INTO edges(parent_id,child_id) VALUES(?1,?2)");
        for (size_t index = 0; index < children.size(); ++index) {
            const SolverState child = globalized(children[index]);
            const int64_t child_id = impl->insert_or_find_node(child, priorities[index]);
            sqlite3_reset(edge.get());
            sqlite3_clear_bindings(edge.get());
            sqlite3_bind_int64(edge.get(), 1, node_id);
            sqlite3_bind_int64(edge.get(), 2, child_id);
            edge.done();
        }

        Statement result(impl->db,
            "INSERT OR IGNORE INTO node_results(node_id,score,trace_id) VALUES(?1,?2,?3)");
        for (const FinishedSolve* route : retained_terminals) {
            sqlite3_reset(result.get());
            sqlite3_clear_bindings(result.get());
            sqlite3_bind_int64(result.get(), 1, node_id);
            sqlite3_bind_int(result.get(), 2, route->points);
            if (route->trace_id >= local_to_global.size() ||
                (route->trace_id != 0 && local_to_global[route->trace_id] == 0)) {
                throw std::logic_error("Terminal result references an unknown local trace");
            }
            sqlite3_bind_int64(result.get(), 3, local_to_global[route->trace_id]);
            result.done();
        }
        {
            Statement expanded(impl->db, "UPDATE node_index SET status=1 WHERE id=?1 AND status=0");
            sqlite3_bind_int64(expanded.get(), 1, node_id);
            expanded.done();
        }
        impl->propagate_from(node_id);
        execute(impl->db, "COMMIT");
        impl->local_to_global_trace.clear();
        ++impl->expanded_this_run;
    } catch (...) {
        execute(impl->db, "ROLLBACK");
        throw;
    }
}

std::vector<FinishedSolve> PersistentSearchGraph::best_routes(const size_t limit) const {
    std::vector<FinishedSolve> routes;
    Statement query(impl->db,
        "SELECT score,trace_id FROM node_results r JOIN node_index i ON i.id=r.node_id "
        "WHERE i.scenario_id=?1 ORDER BY score DESC,trace_id LIMIT ?2");
    sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
    sqlite3_bind_int64(query.get(), 2, static_cast<sqlite3_int64>(limit));
    while (query.row()) {
        const uint32_t terminal_trace = static_cast<uint32_t>(
            sqlite3_column_int64(query.get(), 1)
        );
        std::vector<SolverAction> reversed;
        uint32_t trace_id = terminal_trace;
        Statement trace_query(impl->db,
            "SELECT parent_id,action,food,spice,pottery,marble,silk,jewelry,option "
            "FROM traces WHERE scenario_id=?1 AND trace_id=?2");
        while (trace_id != 0) {
            sqlite3_reset(trace_query.get());
            sqlite3_clear_bindings(trace_query.get());
            sqlite3_bind_int64(trace_query.get(), 1, impl->scenario_id);
            sqlite3_bind_int64(trace_query.get(), 2, trace_id);
            if (!trace_query.row()) throw std::runtime_error("Missing persistent result trace");
            SolverAction action;
            action.action = static_cast<game_actions>(sqlite3_column_int(trace_query.get(), 1));
            action.buy = buy_list(
                static_cast<uint8_t>(sqlite3_column_int(trace_query.get(), 2)),
                static_cast<uint8_t>(sqlite3_column_int(trace_query.get(), 3)),
                static_cast<uint8_t>(sqlite3_column_int(trace_query.get(), 4)),
                static_cast<uint8_t>(sqlite3_column_int(trace_query.get(), 5)),
                static_cast<uint8_t>(sqlite3_column_int(trace_query.get(), 6)),
                static_cast<uint8_t>(sqlite3_column_int(trace_query.get(), 7))
            );
            action.option = static_cast<uint8_t>(sqlite3_column_int(trace_query.get(), 8));
            reversed.push_back(action);
            trace_id = static_cast<uint32_t>(sqlite3_column_int64(trace_query.get(), 0));
        }
        std::reverse(reversed.begin(), reversed.end());
        routes.push_back(FinishedSolve{
            sqlite3_column_int(query.get(), 0), std::move(reversed), terminal_trace,
            FinishedSolve::Origin::Discovered,
        });
    }
    return routes;
}

PersistentSearchStats PersistentSearchGraph::stats() const {
    PersistentSearchStats result;
    Statement nodes(impl->db,
        "SELECT COUNT(*),SUM(status=0),SUM(status=1),SUM(status=2) "
        "FROM node_index WHERE scenario_id=?1");
    sqlite3_bind_int64(nodes.get(), 1, impl->scenario_id);
    nodes.row();
    result.nodes = static_cast<uint64_t>(sqlite3_column_int64(nodes.get(), 0));
    result.open_nodes = static_cast<uint64_t>(sqlite3_column_int64(nodes.get(), 1));
    result.partial_nodes = static_cast<uint64_t>(sqlite3_column_int64(nodes.get(), 2));
    result.solved_nodes = static_cast<uint64_t>(sqlite3_column_int64(nodes.get(), 3));
    {
        Statement query(impl->db,
            "SELECT COUNT(*) FROM edges e JOIN node_index i ON i.id=e.parent_id WHERE i.scenario_id=?1");
        sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
        query.row(); result.edges = static_cast<uint64_t>(sqlite3_column_int64(query.get(), 0));
    }
    {
        Statement query(impl->db,
            "SELECT COUNT(*) FROM node_results r JOIN node_index i ON i.id=r.node_id WHERE i.scenario_id=?1");
        sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
        query.row(); result.terminal_routes = static_cast<uint64_t>(sqlite3_column_int64(query.get(), 0));
    }
    {
        Statement query(impl->db, "SELECT COUNT(*) FROM traces WHERE scenario_id=?1");
        sqlite3_bind_int64(query.get(), 1, impl->scenario_id);
        query.row(); result.trace_links = static_cast<uint64_t>(sqlite3_column_int64(query.get(), 0));
    }
    {
        Statement query(impl->db, "SELECT status,best_score FROM node_index WHERE id=?1");
        sqlite3_bind_int64(query.get(), 1, impl->root_id);
        query.row();
        result.root_solved = sqlite3_column_int(query.get(), 0) == NODE_SOLVED;
        if (sqlite3_column_type(query.get(), 1) != SQLITE_NULL) {
            result.root_best_score = sqlite3_column_int(query.get(), 1);
        }
    }
    result.expanded_this_run = impl->expanded_this_run;
    result.reused_children_this_run = impl->reused_children_this_run;
    std::error_code error;
    result.database_bytes = std::filesystem::file_size(impl->database_path, error);
    error.clear();
    const std::filesystem::path wal = impl->database_path.string() + "-wal";
    if (std::filesystem::exists(wal, error)) {
        error.clear();
        result.database_bytes += std::filesystem::file_size(wal, error);
    }
    return result;
}
