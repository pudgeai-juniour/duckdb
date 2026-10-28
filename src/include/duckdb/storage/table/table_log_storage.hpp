#pragma once

#include "duckdb/common/common.hpp"

#include <span>

namespace duckdb {

class ClientContext;
class DataChunk;

struct WALFrozenRowPositionMap {
	uint64_t source = 0;
	uint64_t target = 0;
	uint32_t base = 0;
	uint32_t rank_begin = 0;
	uint32_t rank_end = 0;
	std::span<const data_t> mask;
};

class TableLogStorage {
public:
	virtual ~TableLogStorage() = default;

	virtual void ReplayInsert(ClientContext &context, DataChunk &chunk) = 0;
	virtual void ReplayTruncate() = 0;
	virtual void ReplayAdoptSegments(vector<string> segments) = 0;
	virtual void ReplayDeleteRowPositions(vector<uint64_t> segments, vector<std::span<const data_t>> docs) = 0;
	virtual void ReplayRowPositionMaps(vector<WALFrozenRowPositionMap> maps) = 0;
};

} // namespace duckdb
