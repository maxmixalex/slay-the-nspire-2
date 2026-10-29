#pragma once

namespace spire {
struct State;
namespace state_workspace {
// A saved title preview and a synchronous console rollback never run at the
// same time. They share one State without using the small calculator stack.
// A read holds the workspace until finish_saved_read; debug transactions
// reject that interval. Only a completed read is visible to its App owner.
State* begin_saved_read(const void* owner);
bool finish_saved_read(const void* owner,bool valid);
const State* saved_probe(const void* owner);
void invalidate_saved_probe(const void* owner);
}
}
