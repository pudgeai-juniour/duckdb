#include "duckdb/catalog/job_schedule.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/parser/parsed_data/bound_pragma_info.hpp"
#include "duckdb/parser/parsed_data/create_job_info.hpp"
#include "duckdb/parser/statement/explain_statement.hpp"
#include "duckdb/parser/statement/pragma_statement.hpp"
#include "duckdb/planner/binder.hpp"
#include "duckdb/planner/expression_binder/constant_binder.hpp"

namespace duckdb {

static Value FoldJobInterval(Binder &binder, ClientContext &context, unique_ptr<ParsedExpression> &expr) {
	ConstantBinder constant_binder(binder, context, "job schedule");
	auto bound = constant_binder.Bind(expr);
	if (bound->HasParameter()) {
		throw NotImplementedException("Job schedules cannot have parameters");
	}
	auto value = ExpressionExecutor::EvaluateScalar(context, *bound, true);
	if (value.IsNull()) {
		throw BinderException("Job schedule interval must not be NULL");
	}
	return value.DefaultCastAs(LogicalType::INTERVAL);
}

void Binder::BindJobSchedule(JobSchedule &schedule, unique_ptr<JobScheduleInfo> &parsed_schedule) {
	if (!parsed_schedule) {
		return;
	}
	schedule.kind = parsed_schedule->kind;
	schedule.concurrent = parsed_schedule->concurrent;
	schedule.interval = FoldJobInterval(*this, context, parsed_schedule->interval);
	if (parsed_schedule->offset) {
		schedule.offset = FoldJobInterval(*this, context, parsed_schedule->offset);
	}
	if (parsed_schedule->randomize) {
		schedule.randomize = FoldJobInterval(*this, context, parsed_schedule->randomize);
	}
	parsed_schedule.reset();
}

static void CheckJobBody(Binder &binder, ClientContext &context, SQLStatement &body) {
	auto &statement = body.type == StatementType::EXPLAIN_STATEMENT ? *body.Cast<ExplainStatement>().stmt : body;
	if (statement.type == StatementType::TRANSACTION_STATEMENT) {
		throw ParserException("CREATE JOB body cannot be a transaction statement");
	}
	if (statement.type == StatementType::MULTI_STATEMENT) {
		throw ParserException("CREATE JOB body cannot be a statement that runs as several statements");
	}
	if (statement.type == StatementType::PRAGMA_STATEMENT) {
		auto &pragma = statement.Cast<PragmaStatement>();
		auto info = pragma.info->Copy();
		auto bound = Binder::CreateBinder(context, &binder)->BindPragma(*info, QueryErrorContext(pragma.stmt_location));
		if (bound->function.query) {
			throw ParserException("CREATE JOB body cannot be a statement that runs as other statements");
		}
	}
}

SchemaCatalogEntry &Binder::BindCreateJobInfo(CreateJobInfo &info) {
	auto &schema = BindCreateSchema(info);
	BindJobSchedule(info.schedule, info.parsed_schedule);
	CheckJobBody(*this, context, *info.body);
	info.SetBody(info.body->ToString());
	return schema;
}

} // namespace duckdb
