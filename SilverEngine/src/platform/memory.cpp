#include "platform/os.h"

namespace sv {

#if SV_EDITOR

	constexpr u32 MODULE_NAME_SIZE = 30;

	struct Allocation {
		void* ptr;
		size_t size;
		char file[FILEPATH_SIZE + 1u];
		u32 line;

		Allocation* next;
	};

	struct MemoryModule {
		char name[MODULE_NAME_SIZE + 1u];
		static constexpr u32 TABLE_SIZE = 10000;
		Allocation* table;
		MemoryModule* next;
		u32 allocation_count;
		size_t bytes;
	};

	struct ModuleNameRef {
		void* ptr;
		MemoryModule* module;
		ModuleNameRef* next;
	};
	
	struct MemoryProfilerData {

		static constexpr u32 TABLE_SIZE = 1000;
		MemoryModule table[TABLE_SIZE];

		static constexpr u32 NAME_TABLE_SIZE = 50000;
		ModuleNameRef name_table[NAME_TABLE_SIZE];
		
		Mutex mutex;
	};

	static MemoryProfilerData* memory_profiler = NULL;

	void _initialize_memory_profiler()
	{
		memory_profiler = (MemoryProfilerData*)_os_allocate_memory(sizeof(MemoryProfilerData));

		mutex_create(memory_profiler->mutex);
	}

	SV_INTERNAL void free_allocation(Allocation& alloc)
	{
		sv::printf("Leak: '%s' Line %u\n", alloc.file, alloc.line);
		if (alloc.next) {
			free_allocation(*alloc.next);
			_os_free_memory(alloc.next);
		}
	}

	SV_INTERNAL void close_module(MemoryModule& module)
	{
		if (module.allocation_count)
			sv::printf("Memory Module '%s'\n", module.name);
		foreach(i, MemoryModule::TABLE_SIZE) {

			if (module.table[i].ptr) {
				free_allocation(module.table[i]);
			}
		}

		_os_free_memory(module.table);

		if (module.allocation_count)
			sv::printf("%s: %lu leaks, %lu bytes\n", module.name, module.allocation_count, module.bytes);
		
		if (module.next) {
			close_module(*module.next);
			_os_free_memory(module.next);
		}
	}

	void _close_memory_profiler()
	{
		if (memory_profiler) {

			mutex_destroy(memory_profiler->mutex);

			foreach(i, MemoryProfilerData::TABLE_SIZE) {

				MemoryModule& module = memory_profiler->table[i];
				if (module.table)
					close_module(module);
			}

			_os_free_memory(memory_profiler);
			memory_profiler = NULL;
		}
	}

	SV_AUX void register_allocation(void* ptr, size_t size, const char* module_name, const char* file, u32 line)
	{
		if (memory_profiler) {
			
			mutex_lock(memory_profiler->mutex);

			u32 module_index = (u32)(size_t(hash_string(module_name)) % (size_t)MemoryProfilerData::TABLE_SIZE);

			MemoryModule* module = memory_profiler->table + module_index;

			if (module->name[0] == '\0') string_copy(module->name, module_name, MODULE_NAME_SIZE + 1u);

			while(!string_equals(module->name, module_name)) {

				if (module->next == NULL) {
					module->next = (MemoryModule*)_os_allocate_memory(sizeof(MemoryModule));
					string_copy(module->next->name, module_name, MODULE_NAME_SIZE + 1u);
				}

				module = module->next;
			}

			++module->allocation_count;
			module->bytes += size;

			u32 alloc_index = (u32)(size_t(ptr) % (size_t)MemoryModule::TABLE_SIZE);

			if (module->table == NULL)
				module->table = (Allocation*)_os_allocate_memory(sizeof(Allocation) * MemoryModule::TABLE_SIZE);

			Allocation* alloc = module->table + alloc_index;

			if (alloc->ptr == NULL) alloc->ptr = ptr;
			
			while (alloc->ptr != ptr) {

				if (alloc->next == NULL) {
					
					alloc->next = (Allocation*)_os_allocate_memory(sizeof(Allocation));
					alloc->next->ptr = ptr;
				}

				alloc = alloc->next;
			}

			alloc->ptr = ptr;
			alloc->size = size;
			string_copy(alloc->file, file, FILEPATH_SIZE + 1u);
			alloc->line = line;

			u32 name_index = (u32)(size_t(ptr) % (size_t)MemoryProfilerData::NAME_TABLE_SIZE);
			ModuleNameRef* ref = memory_profiler->name_table + name_index;

			if (ref->ptr == NULL) ref->ptr = ptr;

			while (ref->ptr != ptr) {
				
				if (ref->next == NULL) {
					ref->next = (ModuleNameRef*)_os_allocate_memory(sizeof(ModuleNameRef));
					ref->next->ptr = ptr;
				}

				ref = ref->next;
			}

			ref->module = module;
			
			mutex_unlock(memory_profiler->mutex);
		}
	}

	SV_AUX void unregister_allocation(void* ptr)
	{
		if (memory_profiler) {

			mutex_lock(memory_profiler->mutex);

			u32 name_index = (u32)(size_t(ptr) % (size_t)MemoryProfilerData::NAME_TABLE_SIZE);
			ModuleNameRef* ref = memory_profiler->name_table + name_index;
			ModuleNameRef* ref_parent = NULL;

			if (ref->ptr == NULL) ref = NULL;

			while (ref && ref->ptr != ptr) {
				
				//SV_ASSERT(ref->next != NULL);
				ref_parent = ref;
				ref = ref->next;
			}

			if (ref) {

				MemoryModule* module = ref->module;
				
				if (ref_parent) {
					ref_parent->next = ref->next;
					_os_free_memory(ref);
				}
				else {

					if (ref->next) {
						*ref = *ref->next;
					}
					else {
						ref->ptr = NULL;
						ref->module = NULL;
						ref->next = NULL;
					}
				}

				u32 alloc_index = (u32)(size_t(ptr) % (size_t)MemoryModule::TABLE_SIZE);
				Allocation* alloc = module->table + alloc_index;
				Allocation* alloc_parent = NULL;

				if (alloc->ptr == NULL) alloc = NULL;

				while (alloc && alloc->ptr != ptr) {

					SV_ASSERT(alloc->next != NULL);
					alloc_parent = alloc;
					alloc = alloc->next;
				}

				if (module->allocation_count)
					--module->allocation_count;
				else SV_ASSERT(0);

				if (module->bytes >= alloc->size)
					module->bytes -= alloc->size;
				else SV_ASSERT(0);

				if (alloc) {

					if (alloc_parent) {
						alloc_parent->next = alloc->next;
						_os_free_memory(alloc);
					}
					else {

						if (alloc->next) {
							*alloc = *alloc->next;
						}
						else {
							alloc->ptr = NULL;
							alloc->file[0] = '\0';
							alloc->line = 0u;
							alloc->next = NULL;
						}
					}
				}
			}
			
			mutex_unlock(memory_profiler->mutex);
		}
	}

	SV_AUX bool find_allocation(void* ptr, MemoryModule** out_module, Allocation** out_alloc)
	{
		u32 name_index = (u32)(size_t(ptr) % (size_t)MemoryProfilerData::NAME_TABLE_SIZE);
		ModuleNameRef* ref = memory_profiler->name_table + name_index;

		if (ref->ptr == NULL) ref = NULL;

		while (ref && ref->ptr != ptr) {
				
			//SV_ASSERT(ref->next != NULL);
			ref = ref->next;
		}

		if (ref) {

			MemoryModule* module = ref->module;
				
			u32 alloc_index = (u32)(size_t(ptr) % (size_t)MemoryModule::TABLE_SIZE);
			Allocation* alloc = module->table + alloc_index;

			if (alloc->ptr == NULL) alloc = NULL;

			while (alloc && alloc->ptr != ptr) {

				SV_ASSERT(alloc->next != NULL);
				alloc = alloc->next;
			}

			if (alloc) {
				*out_module = module;
				*out_alloc = alloc;
				return true;
			}
		}

		return false;
	}
	
	void* _allocate_memory(size_t size, const char* module_name, const char* file, u32 line)
	{
		void* ptr = _os_allocate_memory(size);
		register_allocation(ptr, size, module_name, file, line);
		return ptr;
	}

	void* _reallocate_memory(void* ptr, size_t size)
	{
		ptr = _os_reallocate_memory(ptr, size);

		if (ptr == NULL) unregister_allocation(ptr);
		else {

			mutex_lock(memory_profiler->mutex);

			MemoryModule* module;
			Allocation* alloc;

			if (find_allocation(ptr, &module, &alloc)) {

				module->bytes = module->bytes - alloc->size + size;
				alloc->size = size;
			}

			mutex_unlock(memory_profiler->mutex);
		}

		return ptr;
	}
	
    void _free_memory(void* ptr)
	{
		unregister_allocation(ptr);
		_os_free_memory(ptr);
	}

#else

	void* _allocate_memory(size_t size)
	{
		return _os_allocate_memory(size);
	}

	void* _reallocate_memory(void* ptr, size_t size)
	{
		return _os_reallocate_memory(ptr, size);
	}
	
    void _free_memory(void* ptr)
	{
		_os_free_memory(ptr);
	}

#endif
	
}
