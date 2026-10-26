#include "catch.hpp"
#include "test_helpers.hpp"
#include "duckdb/common/exception/conversion_exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/main/extension/extension_loader.hpp"

using namespace duckdb;

static constexpr char FROM_PAIR[] = "from pair";
static constexpr char FROM_ANY_STRUCT[] = "from any struct";

template <const char *MESSAGE>
static BoundCastInfo Refuse(BindCastInput &, const LogicalType &, const LogicalType &) {
	throw ConversionException(MESSAGE);
}

static void RequireCast(Connection &con, const string &query, const string &message) {
	auto result = con.Query(query);
	INFO(query);
	REQUIRE(result->HasError());
	INFO(result->GetError());
	REQUIRE(StringUtil::Contains(result->GetError(), message));
}

static void IsPair(DataChunk &args, ExpressionState &, Vector &result) {
	auto writer = FlatVector::Writer<bool>(result, args.size());
	for (idx_t row = 0; row < args.size(); row++) {
		writer.WriteValue(true);
	}
}

TEST_CASE("A cast registered from a type does not stop the type's own values from binding", "[api][cast]") {
	DuckDB db(nullptr);
	ExtensionLoader loader(*db.instance, "cast_lookup");
	auto pair = LogicalType::STRUCT({{"a", LogicalType::INTEGER}}).WithAlias("pair");
	loader.RegisterType("pair", pair);
	loader.RegisterCastFunction(pair, LogicalType::STRUCT({{"any", LogicalType::ANY}}), Refuse<FROM_PAIR>);
	loader.RegisterFunction(ScalarFunction("is_pair", {pair}, LogicalType::BOOLEAN, IsPair));
	Connection con(db);

	auto result = con.Query("SELECT is_pair({'a': 1}::pair)");
	REQUIRE(CHECK_COLUMN(result, 0, {true}));
	RequireCast(con, "SELECT {'a': 1}::pair::STRUCT(b INTEGER)", FROM_PAIR);
}

TEST_CASE("A type's casts fall back to the casts registered for its family", "[api][cast]") {
	DuckDB db(nullptr);
	ExtensionLoader loader(*db.instance, "cast_lookup");
	auto pair = LogicalType::STRUCT({{"a", LogicalType::INTEGER}}).WithAlias("pair");
	loader.RegisterType("pair", pair);
	loader.RegisterCastFunction(pair, LogicalType::VARCHAR, Refuse<FROM_PAIR>);
	loader.RegisterCastFunction(LogicalType::STRUCT({{"any", LogicalType::ANY}}), LogicalType::UUID,
	                            Refuse<FROM_ANY_STRUCT>);
	Connection con(db);

	RequireCast(con, "SELECT {'a': 1}::pair::VARCHAR", FROM_PAIR);
	RequireCast(con, "SELECT {'a': 1}::pair::UUID", FROM_ANY_STRUCT);
	RequireCast(con, "SELECT {'a': 1}::UUID", FROM_ANY_STRUCT);
}
