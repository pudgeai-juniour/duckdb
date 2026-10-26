#pragma once

#include "duckdb/common/common.hpp"

#include <span>

namespace duckdb {

class ClientContext;
class DataChunk;

struct WALFrozenRowPositionMap {
	uint32_t source = 0;
	uint32_t target = 0;
	std::span<const data_t> docs;
};

class TableLogStorage {
public:
	virtual ~TableLogStorage() = default;

	virtual void ReplayInsert(ClientContext &context, DataChunk &chunk) = 0;
	virtual void ReplayTruncate() = 0;
	virtual void ReplayAdoptSegments(vector<string> segments) = 0;
	virtual void ReplayDeleteRowPositions(std::span<const uint32_t> segments,
	                                      std::span<const std::span<const data_t>> docs) = 0;
	virtual void ReplayRowPositionMaps(std::span<const WALFrozenRowPositionMap> maps) = 0;
};

} // namespace duckdb
