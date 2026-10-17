#include "duckdb/parser/statement/multi_statement.hpp"

namespace duckdb {

MultiStatement::MultiStatement() : SQLStatement(StatementType::MULTI_STATEMENT) {
}

MultiStatement::MultiStatement(const MultiStatement &other) : SQLStatement(other), source_sql(other.source_sql) {
	for (auto &stmt : other.statements) {
		statements.push_back(stmt->Copy());
	}
}

unique_ptr<SQLStatement> MultiStatement::Copy() const {
	return unique_ptr<MultiStatement>(new MultiStatement(*this));
}

string MultiStatement::ToString() const {
	if (!source_sql.empty()) {
		return source_sql;
	}
	return StringUtil::Join(statements, " ", [](const auto &stmt) {
		auto text = stmt->ToString();
		return StringUtil::EndsWith(text, ";") ? text : text + ";";
	});
}

} // namespace duckdb
