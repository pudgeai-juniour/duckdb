#include "duckdb/parser/parsed_data/create_job_info.hpp"

#include "duckdb/common/enum_util.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/parser/statement/multi_statement.hpp"

namespace duckdb {

unique_ptr<SQLStatement> CreateJobInfo::ParseBody(const string &sql) {
	auto parser = Parser::GetBuiltinParser();
	parser.ParseQuery(sql);
	if (parser.statements.empty()) {
		throw ParserException("Job body has no statement");
	}
	for (auto &statement : parser.statements) {
		if (IsScript(*statement)) {
			throw ParserException("CREATE JOB body cannot contain a statement that runs as several statements");
		}
	}
	if (parser.statements.size() == 1) {
		return std::move(parser.statements[0]);
	}
	auto script = make_uniq<MultiStatement>();
	script->statements = std::move(parser.statements);
	return std::move(script);
}

bool CreateJobInfo::IsScript(const SQLStatement &body) {
	return body.type == StatementType::MULTI_STATEMENT && body.Cast<MultiStatement>().source_sql.empty();
}

void CreateJobInfo::SetBody(const string &sql) {
	body = ParseBody(sql);
}

unique_ptr<CreateInfo> CreateJobInfo::Copy() const {
	auto result = make_uniq<CreateJobInfo>();
	CopyProperties(*result);
	result->schedule = schedule;
	result->suspended = suspended;
	result->body = body->Copy();
	result->parsed_schedule = parsed_schedule ? parsed_schedule->Copy() : nullptr;
	return std::move(result);
}

string CreateJobInfo::ScheduleToString(const JobSchedule &schedule,
                                       const unique_ptr<JobScheduleInfo> &parsed_schedule) {
	if (!parsed_schedule) {
		return schedule.ToString();
	}
	string result = EnumUtil::ToString(parsed_schedule->kind) + " (";
	result += parsed_schedule->interval->ToString() + ")";
	if (parsed_schedule->offset) {
		result += " OFFSET (" + parsed_schedule->offset->ToString() + ")";
	}
	if (parsed_schedule->randomize) {
		result += " RANDOMIZE FOR (" + parsed_schedule->randomize->ToString() + ")";
	}
	if (parsed_schedule->concurrent) {
		result += " CONCURRENT";
	}
	return result;
}

string CreateJobInfo::ToString() const {
	string result = "CREATE";
	if (on_conflict == OnCreateConflict::REPLACE_ON_CONFLICT) {
		result += " OR REPLACE";
	}
	if (temporary) {
		result += " TEMPORARY";
	}
	result += " JOB ";
	if (on_conflict == OnCreateConflict::IGNORE_ON_CONFLICT) {
		result += "IF NOT EXISTS ";
	}
	result += QualifiedNameToString();
	result += " " + ScheduleToString(schedule, parsed_schedule);
	if (suspended) {
		result += " SUSPENDED";
	}
	auto text = body->ToString();
	if (!IsScript(*body)) {
		return result + " AS " + text + ";";
	}
	string tag = "$$";
	for (idx_t i = 0; text.find(tag) != string::npos; i++) {
		tag = "$job" + to_string(i) + "$";
	}
	return result + " AS " + tag + text + tag + ";";
}

} // namespace duckdb
