#include "core.h"

namespace sv {
    
    void DoUndoStack::push_action(ActionFn do_fn, ActionFn undo_fn)
    {
	if (_stack_pos != _stack.size()) {

	    _buffer.resize(_stack[_stack_pos].buffer_offset);
	}
	
	Action action;
	action.do_fn = do_fn;
	action.undo_fn = undo_fn;
	action.buffer_offset = _buffer.size();
	
	_stack.resize(_stack_pos + 1u);
	_stack[_stack_pos] = action;
    }
    
    void DoUndoStack::push_data(const void* data, size_t size)
    {
	SV_ASSERT(_stack.size());
	_buffer.write_back(data, size);
    }

    bool DoUndoStack::do_action(void* return_data)
    {
	bool done = false;
	
	if (_stack_pos < _stack.size()) {

	    Action action = _stack[_stack_pos];
	    if (action.do_fn) {

		void* ptr = _buffer.data() + action.buffer_offset;
		action.do_fn(ptr, return_data);
		done = true;
	    }
	    ++_stack_pos;
	}

	return done;
    }

    bool DoUndoStack::undo_action(void* return_data)
    {
	bool done = false;
	
	if (_stack_pos != 0u) {

	    --_stack_pos;
	    
	    Action action = _stack[_stack_pos];
	    if (action.undo_fn) {

		void* ptr = _buffer.data() + action.buffer_offset;
		action.undo_fn(ptr, return_data);
		done = true;
	    }
	}
	
	return done;
    }

    void DoUndoStack::clear()
    {
	_stack.clear();
	_buffer.clear();
	_stack_pos = 0u;
    }
}
