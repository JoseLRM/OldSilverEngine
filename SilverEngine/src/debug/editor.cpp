#if SV_EDITOR

#include "core/renderer.h"
#include "debug/console.h"
#include "core/event_system.h"
#include "core/physics3D.h"

namespace sv {

	enum HierarchyElementType : u32 {
		HierarchyElementType_Entity,
		HierarchyElementType_Folder,
		HierarchyElementType_Root,
	};

	struct HierarchyElement {
		HierarchyElementType type;
		List<HierarchyElement> elements;

		Entity entity;
		char folder_name[ENTITY_NAME_SIZE + 1];
		u32 folder_id;
	};

	struct EntityHierarchyData {
		HierarchyElement root;
		u32 folder_id_count;
		
		u32 package_folder_id = 0;
		HierarchyElementPackage package;
	};

	enum GizmosTransformMode : u32 {
		GizmosTransformMode_None,
		GizmosTransformMode_Position,
		GizmosTransformMode_Scale
    };
	
    constexpr f32 GIZMOS_SIZE = 0.1f;

    enum GizmosObject : u32 {
		GizmosObject_None,
		GizmosObject_AxisX,
		GizmosObject_AxisY,
		GizmosObject_AxisZ,
    };

    struct GizmosData {

		GizmosTransformMode mode = GizmosTransformMode_Position;
		f32 position_advance = 0.5f;
		f32 scale_advance = 0.5f;
		f32 rotation_advance = PI / 4.f;

		GizmosObject object = GizmosObject_None;
		bool focus = false;
	
		v3_f32 start_offset;
		v3_f32 start_scale;
		f32 axis_size;
		f32 selection_size;
    };

	struct TerrainBrushData {
		f32 strength = 10.f;
		f32 range = 10.f;
		f32 min_height = 0.f;
		f32 max_height = 1000.f;
	};

	enum EditorToolType : u32 {
		EditorToolType_Gizmos,
		EditorToolType_TerrainBrush,
	};
	
	struct EditorToolData {
		EditorToolType tool_type = EditorToolType_Gizmos;
		GizmosData gizmos_data;
		TerrainBrushData terrain_brush_data;
	};

    enum AssetElementType : u32 {
		AssetElementType_Unknown,
		AssetElementType_Texture,
		AssetElementType_Mesh,
		AssetElementType_Material,
		AssetElementType_SpriteSheet,
		AssetElementType_Sound,
		AssetElementType_Prefab,
		AssetElementType_Entity,
		AssetElementType_Directory,
    };

    struct AssetElement {
		char name[FILENAME_SIZE + 1];
		AssetElementType type;
    };

    struct AssetBrowserInfo {
		char filepath[FILEPATH_SIZE + 1] = {};
		List<AssetElement> elements;
		f64 last_update = 0.0;

		struct {

			char assetname[FILENAME_SIZE + 1u] = "";
			u32 state = 0u;
			
		} create_asset_data;
    };

	enum SpriteSheetEditorState : u32 {
		SpriteSheetEditorState_Main,
		SpriteSheetEditorState_SpriteList,
		SpriteSheetEditorState_ModifySprite,
		SpriteSheetEditorState_NewSprite,
		SpriteSheetEditorState_AnimationList,
		SpriteSheetEditorState_NewAnimation,
		SpriteSheetEditorState_ModifyAnimation,
		SpriteSheetEditorState_AddSprite,
	};

    struct SpriteSheetEditorData {
		SpriteSheetAsset current_sprite_sheet;
		SpriteSheetEditorState state = SpriteSheetEditorState_Main;
		SpriteSheetEditorState next_state = SpriteSheetEditorState_Main;
		Sprite temp_sprite;
		SpriteAnimation temp_anim;
		u32 modifying_id = 0u;
		f32 simulation_time = 0.f;
    };

	struct MaterialEditorData {
		MaterialAsset material;
	};

	struct ImportModelData {
		ModelInfo model_info;
	};

	struct CreateTagData {
		char name[TAG_NAME_SIZE + 1u] = "";
	};

    struct GlobalEditorData {

		List<Entity> selected_entities;
		bool camera_focus = false;
		bool entity_viewer = false;
		bool debug_draw = true;

		v2_f32 absolute_mouse_position;
		v2_f32 absolute_mouse_last_position;
		v2_f32 absolute_mouse_dragged;
		
		v2_f32 editor_view_size;
		bool in_editor_view;
		v2_f32 game_view_size;
		bool in_game_view;
		v2_f32 view_position;

		AssetBrowserInfo asset_browser;

		EditorToolData tool_data;
		
		bool show_editor = false;
		bool show_game = false;
		GPUImage* offscreen_editor = NULL;
		GPUImage* offscreen_game = NULL;
	
		TextureAsset image;
		static constexpr v4_f32 TEXCOORD_FOLDER = { 0.f, 0.f, 0.05989583333333f, 0.05989583333333f };
		static constexpr v4_f32 TEXCOORD_FILE = { 2.f / 1920.f, 105.f / 1920.f, 109.f / 1920.f, 212.f / 1920.f };
		static constexpr v4_f32 TEXCOORD_PLAY = { 141.f / 1920.f, 6.f / 1920.f, 264.f / 1920.f, 129.f / 1920.f };
		static constexpr v4_f32 TEXCOORD_PAUSE = { 275.f / 1920.f, 5.f / 1920.f, 386.f / 1920.f, 116.f / 1920.f };
		static constexpr v4_f32 TEXCOORD_STOP = { 405.f / 1920.f, 6.f / 1920.f, 514.f / 1920.f, 115.f / 1920.f };
		static constexpr v4_f32 TEXCOORD_LIGHT_PROBE = { 1402.f / 1920.f, 1402.f / 1920.f, 1.f, 1.f };
		static constexpr v4_f32 TEXCOORD_CAMERA_PROBE = { 784.f / 1920.f, 1320.f / 1920.f, 1338.f / 1920.f, 1874 / 1920.f };

		EntityHierarchyData entity_hierarchy_data;
		SpriteSheetEditorData sprite_sheet_editor_data;
		MaterialEditorData material_editor_data;
		ImportModelData import_model_data;
		CreateTagData create_tag_data;

		char next_scene_name[SCENE_NAME_SIZE + 1u] = "";
    };

    GlobalEditorData editor;

	/////////////////////////////////////////////// DO UNDO ACTIONS ////////////////////////////////////
    
    typedef void(*ConstructEntityActionFn)(Entity entity);
    
    struct EntityCreate_Action {
		Entity entity;
		Entity parent;
		ConstructEntityActionFn construct_entity;
    };
    
    SV_INTERNAL void do_entity_create(void* pdata, void* preturn) {

		EntityCreate_Action& data = *(EntityCreate_Action*)pdata;

		Entity parent = data.parent;
		if (!entity_exists(parent))
			parent = 0;

		const char* name = (const char*)(pdata) + sizeof(data);
		if (*name == '\0')
			name = nullptr;
	
		data.entity = create_entity(parent, name);

		if (preturn) {
			memcpy(preturn, &data.entity, sizeof(Entity));
		}

		if (data.construct_entity) {
			data.construct_entity(data.entity);
		}
    }
    SV_INTERNAL void undo_entity_create(void* pdata, void* preturn) {

		EntityCreate_Action& data = *(EntityCreate_Action*)pdata;
		if (entity_exists(data.entity))
			destroy_entity(data.entity);
    }
    
    SV_INTERNAL void construct_entity_sprite(Entity entity) {
		add_entity_component(entity, get_component_id("Sprite"));
    }
	SV_INTERNAL void construct_entity_cube(Entity entity) {
		MeshComponent* mesh = (MeshComponent*)add_entity_component(entity, get_component_id("Mesh"));

		if (mesh) {
			create_asset_from_name(mesh->mesh, "Mesh", "Cube");
		}
    }
	SV_INTERNAL void construct_entity_sphere(Entity entity) {
		MeshComponent* mesh = (MeshComponent*)add_entity_component(entity, get_component_id("Mesh"));

		if (mesh) {
			create_asset_from_name(mesh->mesh, "Mesh", "Sphere");
		}
    }
    SV_INTERNAL void construct_entity_camera(Entity entity) {
		CameraComponent* camera = (CameraComponent*)add_entity_component(entity, get_component_id("Camera"));
		if (camera){
			camera->projection_type = ProjectionType_Perspective;
			camera->near = 0.2f;
			camera->far = 10000.f;
			camera->width = 0.9f;
			camera->height = 0.9f;
		}
    }
	SV_INTERNAL void construct_entity_2D_camera(Entity entity) {
		add_entity_component(entity, get_component_id("Camera"));
    }
    
    SV_INTERNAL Entity editor_create_entity(Entity parent = 0, const char* name = nullptr, ConstructEntityActionFn construct_entity = nullptr)
    {
		DoUndoStack& stack = dev.do_undo_stack;
		stack.lock();

		stack.push_action(do_entity_create, undo_entity_create);
	
		EntityCreate_Action data;
		data.entity = 0;
		data.parent = parent;
		data.construct_entity = construct_entity;
	
		stack.push_data(&data, sizeof(data));

		size_t name_size = name ? strlen(name) : 0u;
		if (name_size)
			stack.push_data(name, name_size + 1u);
		else {
			char c = '\0';
			stack.push_data(&c, 1u);
		}

		Entity res;
		stack.do_action(&res);
	
		stack.unlock();

		return res;
    }

	/////////////////// ENTITY SELECTION ///////////////////

	SV_AUX bool is_entity_selected(Entity entity)
	{
		for (Entity e : editor.selected_entities)
			if (e == entity) return true;
		return false;
	}

	SV_AUX void unselect_entities()
	{
		editor.selected_entities.reset();
	}

	SV_AUX void select_entity(Entity entity, bool unselect_all)
	{
		if (unselect_all) unselect_entities();
		else if (is_entity_selected(entity)) return;

		if (entity_exists(entity))
			editor.selected_entities.push_back(entity);
	}

	SV_AUX void unselect_entity(Entity entity)
	{
		foreach (i, editor.selected_entities.size()) {
			if (editor.selected_entities[i] == entity) {
				editor.selected_entities.erase(i);
				break;
			}
		}
	}

	SV_AUX void edit_sprite_sheet(const char* filepath)
	{
		gui_show_window("SpriteSheet Editor");
		SpriteSheetAsset& asset = editor.sprite_sheet_editor_data.current_sprite_sheet;
		if (!load_asset_from_file(asset, filepath)) {

			SV_LOG_ERROR("Unknown error loading '%s'", filepath);
			gui_hide_window("SpriteSheet Editor");
		}
	}

	/////////////////// ENTITY HIERARCHY ///////////////////

	inline u64 compute_hierarchy_state_hash(const char* scene_name)
	{
		u64 hash = 93742934789123;
		hash_combine(hash, hash_string(scene_name));
		return hash;
	}

	static void deserialize_element(Deserializer& s, HierarchyElement& element)
	{
		deserialize_u32(s, (u32&)element.type);

		if (element.type == HierarchyElementType_Entity) {
			deserialize_u32(s, element.entity);
		}
		else if (element.type == HierarchyElementType_Folder) {
			deserialize_u32(s, element.folder_id);
			deserialize_string(s, element.folder_name, ENTITY_NAME_SIZE + 1);
		}

		u32 element_count;
		deserialize_u32(s, element_count);

		element.elements.resize(element_count);

		foreach(i, element_count) {
			deserialize_element(s, element.elements[i]);
		}
	}

	static void load_hierarchy_state(InitializeSceneEvent* e)
	{
		u64 hash = compute_hierarchy_state_hash(e->name);
		auto& data = editor.entity_hierarchy_data;

		Deserializer s;

		if (bin_read(hash, s)) {

			u32 version;
			deserialize_u32(s, version);

			deserialize_element(s, data.root);

			deserialize_u32(s, data.folder_id_count);

			deserialize_end(s);
		}
		else {
			SV_LOG_WARNING("Entity Hierarchy state not found");
		}
	}

	static void validate_element(HierarchyElement& element)
	{
		for(u32 i = 0u; i < element.elements.size();) {

			auto& e = element.elements[i];
			
			if (e.type == HierarchyElementType_Entity && !entity_exists(e.entity)) {
				element.elements.erase(i);
			}
			else {
				++i;
			}
		}

		for (HierarchyElement& e : element.elements) {
			validate_element(e);
		}
	}

	static void validate_hierarchy()
	{
		validate_element(editor.entity_hierarchy_data.root);
	}

	static void serialize_element(Serializer& s, const HierarchyElement& element)
	{
		serialize_u32(s, element.type);

		if (element.type == HierarchyElementType_Entity) {
			serialize_u32(s, element.entity);
		}
		else if (element.type == HierarchyElementType_Folder) {
			serialize_u32(s, element.folder_id);
			serialize_string(s, element.folder_name);
		}
		
		serialize_u32(s, (u32)element.elements.size());

		for (const HierarchyElement& e : element.elements) {
			serialize_element(s, e);
		}
	}

	static void save_hierarchy_state(CloseSceneEvent* e)
	{
		u64 hash = compute_hierarchy_state_hash(e->name);
		auto& data = editor.entity_hierarchy_data;

		Serializer s;

		serialize_begin(s);

		serialize_u32(s, 0);

		serialize_element(s, data.root);

		serialize_u32(s, data.folder_id_count);

		if(!bin_write(hash, s)) {

			SV_LOG_ERROR("Can't save the entity hierarchy state");
		}
	}
	
	static HierarchyElement* find_hierarchy_element_with_entity(HierarchyElement& element, Entity entity, bool parent)
	{
		for (HierarchyElement& e : element.elements) {

			if (e.entity == entity) {
				if (parent)
					return &element;
				else
					return &e;
			}

			HierarchyElement* res = find_hierarchy_element_with_entity(e, entity, parent);
			if (res)
				return res;
		}

		return NULL;
	}

	static HierarchyElement* find_hierarchy_element_with_folder(HierarchyElement& element, u32 folder_id, bool parent)
	{
		for (HierarchyElement& e : element.elements) {

			if (e.folder_id == folder_id) {
				if (parent)
					return &element;
				else
					return &e;
			}

			HierarchyElement* res = find_hierarchy_element_with_folder(e, folder_id, parent);
			if (res)
				return res;
		}

		return NULL;
	}

	static HierarchyElement* find_hierarchy_element_with_child(HierarchyElement& element, HierarchyElement* child)
	{
		for (HierarchyElement& e : element.elements) {
			
			if (&e == child) {
				return &element;
			}

			HierarchyElement* res = find_hierarchy_element_with_child(e, child);
			if (res)
				return res;
		}

		return NULL;
	}

	static void create_entity_element(EntityCreateEvent* e)
	{
		auto& data = editor.entity_hierarchy_data;

		if (find_hierarchy_element_with_entity(data.root, e->entity, false))
			return;

		Entity parent = get_entity_parent(e->entity);

		HierarchyElement* element = NULL;
		
		if (parent == 0) {
			
			element = &data.root.elements.emplace_back();
		}
		else {
			
			HierarchyElement* parent_element = find_hierarchy_element_with_entity(data.root, parent, false);
			if (parent_element) {

				element = &parent_element->elements.emplace_back();
			}
			else SV_ASSERT(0);
		}

		if (element) {
			element->type = HierarchyElementType_Entity;
			element->entity = e->entity;
			element->folder_name[0] = '\0';
		}
		else SV_ASSERT(0);
	}

	static void destroy_entity_element(EntityDestroyEvent* e)
	{
		auto& data = editor.entity_hierarchy_data;

		HierarchyElement* parent_element = find_hierarchy_element_with_entity(data.root, e->entity, true);

		if (parent_element) {

			u32 index = u32_max;

			foreach(i, parent_element->elements.size()) {

				HierarchyElement& el = parent_element->elements[i];
				
				if (el.entity == e->entity) {
					index = i;
					break;
				}
			}

			if (index != u32_max) {

				parent_element->elements.erase(index);
			}
		}
	}

	inline void create_editor_folder(HierarchyElement& parent, const char* name)
	{
		HierarchyElement& e = parent.elements.emplace_back();
		e.type = HierarchyElementType_Folder;
		string_copy(e.folder_name, name, ENTITY_NAME_SIZE + 1);
		e.entity = 0;
		e.folder_id = ++editor.entity_hierarchy_data.folder_id_count;
	}

	static void destroy_editor_folder(HierarchyElement& folder)
	{
		SV_ASSERT(folder.type == HierarchyElementType_Folder);

		if (folder.type == HierarchyElementType_Folder) {

			while (folder.elements.size()) {

				HierarchyElement& element = folder.elements.back();

				if (element.type == HierarchyElementType_Folder) {
					destroy_editor_folder(element);
				}
				else if (element.type == HierarchyElementType_Entity) {

					destroy_entity(element.entity);
				}
			}

			HierarchyElement* parent = find_hierarchy_element_with_child(editor.entity_hierarchy_data.root, &folder);

			if (parent) {

				u32 index = u32_max;

				foreach(i, parent->elements.size()) {

					if (parent->elements.data() + i == &folder) {
						index = i;
						break;
					}
				}

				if (index != u32_max) {

					parent->elements.erase(index);
				}
				else SV_ASSERT(0);
			}
			else SV_ASSERT(0);
		}
	}

	static void move_entity_into_folder(u32 folder_id, Entity entity)
	{
		auto& data = editor.entity_hierarchy_data;
		
		if (get_entity_parent(entity) != 0)
			return;

		HierarchyElement* _element = find_hierarchy_element_with_folder(data.root, folder_id, false);
		if (_element == NULL) return;
		
		HierarchyElement& element = *_element;
		
		if (element.type == HierarchyElementType_Folder) {
			
			HierarchyElement* parent_element = find_hierarchy_element_with_entity(data.root, entity, true);

			if (parent_element) {

				u32 index = u32_max;

				foreach(i, parent_element->elements.size()) {

					if (parent_element->elements[i].entity == entity) {
						index = i;
						break;
					}
				}

				if (index != u32_max) {

					element.elements.emplace_back(std::move(parent_element->elements[index]));
					parent_element->elements.erase(index);
				}
			}
		}
	}

	static void move_folder_into_folder(u32 dst_folder_id, u32 folder_id)
	{
		auto& data = editor.entity_hierarchy_data;
		
		HierarchyElement* _element = find_hierarchy_element_with_folder(data.root, dst_folder_id, false);
		if (_element == NULL) return;
		
		HierarchyElement& element = *_element;
		
		if (element.type == HierarchyElementType_Folder) {

			HierarchyElement* parent_element = find_hierarchy_element_with_folder(data.root, folder_id, true);

			if (parent_element) {

				u32 index = u32_max;

				foreach(i, parent_element->elements.size()) {

					if (parent_element->elements[i].folder_id == folder_id) {
						index = i;
						break;
					}
				}

				if (index != u32_max) {

					element.elements.emplace_back(std::move(parent_element->elements[index]));
					parent_element->elements.erase(index);
				}
			}
		}
	}

	static void move_folder_into_root(u32 folder_id)
	{
		auto& data = editor.entity_hierarchy_data;
		
		HierarchyElement& element = data.root;
		
		if (element.type == HierarchyElementType_Root) {

			HierarchyElement* parent_element = find_hierarchy_element_with_folder(data.root, folder_id, true);

			if (parent_element) {

				u32 index = u32_max;

				foreach(i, parent_element->elements.size()) {

					if (parent_element->elements[i].folder_id == folder_id) {
						index = i;
						break;
					}
				}

				if (index != u32_max) {

					element.elements.emplace_back(std::move(parent_element->elements[index]));
					parent_element->elements.erase(index);
				}
			}
		}
	}

	static void move_entity_into_root(Entity entity)
	{
		auto& data = editor.entity_hierarchy_data;
		
		HierarchyElement& element = data.root;
		
		if (element.type == HierarchyElementType_Root) {
			
			HierarchyElement* parent_element = find_hierarchy_element_with_entity(data.root, entity, true);

			if (parent_element) {

				u32 index = u32_max;

				foreach(i, parent_element->elements.size()) {

					if (parent_element->elements[i].entity == entity) {
						index = i;
						break;
					}
				}

				if (index != u32_max) {

					element.elements.emplace_back(std::move(parent_element->elements[index]));
					parent_element->elements.erase(index);
				}
			}
		}
	}

	static void select_folder_entities(HierarchyElement& element)
	{
		if (element.type == HierarchyElementType_Folder) {

			for (HierarchyElement& e : element.elements) {

				if (e.type == HierarchyElementType_Entity) {
					select_entity(e.entity, false);
				}
				else if (e.type == HierarchyElementType_Folder) {
					select_folder_entities(e);
				}
			}
		}
	}

	static void show_element_popup(HierarchyElement& element, bool& destroy)
    {
		if (element.type == HierarchyElementType_Entity) {

			Entity entity = element.entity;
			
			if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {
	    
				if (gui_button("Destroy")) {
					destroy = true;
					gui_close_popup();
				}

				if (gui_button("Duplicate")) {
					duplicate_entity(entity);
				}
	    
				if (gui_button("Create Child")) {
					editor_create_entity(entity);
				}

				if (!is_prefab(entity)) {

					gui_separator(1);

					if (gui_button("Save Prefab")) {
				
						char path[FILEPATH_SIZE + 1u] = "";
		    
						if (file_dialog_save(path, 0u, nullptr, "")) {

							char* extension = filepath_extension(path);
							if (extension != nullptr) {
								*extension = '\0';
							}
							string_append(path, ".prefab", FILEPATH_SIZE + 1u);

							save_prefab(entity, path);
						}
					}
				}
				else {

					if (gui_button("Create Entity")) {

						Entity created = create_entity(0, NULL, entity);

						select_entity(created, true);
					}
				}

				gui_end_popup();
			}
		}
		else if (element.type == HierarchyElementType_Folder) {
			
			if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {
	    
				if (gui_button("Destroy")) {
					destroy = true;
					gui_close_popup();
				}

				if (gui_button("Create Folder")) {
					create_editor_folder(element, "Folder");
				}

				gui_end_popup();
			}
		}
    }

    static void show_element(HierarchyElement& element)
    {
		auto& data = editor.entity_hierarchy_data;
		const char* name = "";
		bool selected = false;
		u64 id = u64_max;
		
		if (element.type == HierarchyElementType_Entity) {

			id = (u64)element.entity;

			name = get_entity_name(element.entity);
			if (string_size(name) == 0u) name = "Unnamed";

			selected = is_entity_selected(element.entity);
		}
		else if (element.type == HierarchyElementType_Folder) {
			
			id = (u64)(element.folder_id | 0xFF000000);

			name = element.folder_name;
			if (string_size(name) == 0u) name = "Unnamed";

			selected = false; // TODO
		}
		else if (element.type == HierarchyElementType_Root) {

			foreach(i, element.elements.size()) {

				HierarchyElement& e = element.elements[i];
				show_element(e);
			}
			return;
		}
		else return;

		bool destroy = false;

		u32 flags = 0u;
		bool has_childs = element.elements.size() != 0;

		if (selected) flags |= GuiTreeFlag_Selected;

		if (has_childs != 0)
			flags |= GuiTreeFlag_Parent;

		if (element.type == HierarchyElementType_Folder)
			gui_push_image(GuiImageType_Icon, editor.image.get(), editor.TEXCOORD_FOLDER);
		bool begin = gui_begin_tree(name, id, flags);
		if (element.type == HierarchyElementType_Folder)
			gui_pop_image();

		HierarchyElementPackage package;

		if (element.type == HierarchyElementType_Entity) {
			
			package.entity = (editor.selected_entities.size() > 1) ? 0 : element.entity;
			package.folder_id = 0;
		}
		else if (element.type == HierarchyElementType_Folder) {
			package.entity = 0;
			package.folder_id = element.folder_id;
		}

		package.type = element.type;
		gui_send_package(&package, sizeof(HierarchyElementPackage), HIERARCHY_ELEMENT_PACKAGE);

		if (element.type == HierarchyElementType_Folder) {

			HierarchyElementPackage* package;
			if (gui_recive_package((void**)&package, HIERARCHY_ELEMENT_PACKAGE, GuiReciverTrigger_LastWidget)) {

				data.package = *package;
				data.package_folder_id = element.folder_id;
			}
		}
		
		if (gui_tree_pressed()) {

			if (element.type == HierarchyElementType_Entity) {

				Entity entity = element.entity;
				
				if (selected) {
					if (editor.selected_entities.size() == 1u)
						unselect_entity(entity);
					else {
						select_entity(entity, true);
					}
				}
				else {
					select_entity(entity, !input.keys[Key_Shift]);
				}
			}
			else if (element.type == HierarchyElementType_Folder) {

				if (!input.keys[Key_Shift])
					unselect_entities();

				select_folder_entities(element);
			}
		}

		show_element_popup(element, destroy);

		if (begin) {

			if (!destroy) {
					
				foreach(i, element.elements.size()) {

					HierarchyElement& e = element.elements[i];
					show_element(e);
				}
			}
			
			gui_end_tree();
		}
		
		if (destroy) {

			if (element.type == HierarchyElementType_Entity) {

				Entity entity = element.entity;
				destroy_entity(entity);
			
				foreach(i, editor.selected_entities.size()) {
				
					if (editor.selected_entities[i] == entity) {
						editor.selected_entities.erase(i);
						break;
					}
				}
			}
			else if (element.type == HierarchyElementType_Folder) {

				destroy_editor_folder(element);
			}
		}
    }

    inline void display_create_popup()
    {	
		gui_push_id("Create Popup");
	
		if (gui_begin_popup(GuiPopupTrigger_Root)) {
		
			if (gui_button("Create Entity")) {
				editor_create_entity();
			}

			if (gui_button("Create Sprite")) {

				editor_create_entity(0, "Sprite", construct_entity_sprite);
			}

			if (gui_button("Create Cube")) {

				editor_create_entity(0, "Cube", construct_entity_cube);
			}

			if (gui_button("Create Sphere")) {

				editor_create_entity(0, "Sphere", construct_entity_sphere);
			}

			if (gui_button("Create Camera")) {

				editor_create_entity(0, "Camera", construct_entity_camera);
			}

			if (gui_button("Create 2D Camera")) {

				editor_create_entity(0, "Camera 2D", construct_entity_2D_camera);
			}

			gui_separator(2);

			if (gui_button("Create Folder")) {

				create_editor_folder(editor.entity_hierarchy_data.root, "Folder");
			}

			gui_end_popup();
		}

		gui_pop_id();
    }
    
    void display_entity_hierarchy()
    {
		auto& data = editor.entity_hierarchy_data;
		
		if (gui_begin_window("Hierarchy")) {

			show_element(data.root);

			display_create_popup();

			// Recive prefabs
			{
				PrefabPackage* package;
				if (gui_recive_package((void**)&package, ASSET_BROWSER_PREFAB, GuiReciverTrigger_Root, GuiReciverStyle_None)) {
				
					load_prefab(package->filepath);
				}
			}

			// Recive hierarchy elements
			{
				HierarchyElementPackage* package;
				if (gui_recive_package((void**)&package, HIERARCHY_ELEMENT_PACKAGE, GuiReciverTrigger_Root, GuiReciverStyle_None)) {
					
					if (package->folder_id != 0) {
						move_folder_into_root(package->folder_id);
					}
					else if (package->entity == 0 || entity_exists(package->entity)) {

						if (package->entity == 0) {

							for (Entity e : editor.selected_entities)
								move_entity_into_root(e);
						}
						else move_entity_into_root(package->entity);
					}
				}
			}
			
			gui_end_window();
		}

		if (data.package_folder_id != 0) {

			if (data.package.folder_id != 0) {

				move_folder_into_folder(data.package_folder_id, data.package.folder_id);
			}
			else if (data.package.entity == 0 || entity_exists(data.package.entity)) {

				if (data.package.entity == 0) {

					for (Entity e : editor.selected_entities)
						move_entity_into_folder(data.package_folder_id, e);
				}
				else move_entity_into_folder(data.package_folder_id, data.package.entity);
			}

			data.package_folder_id = 0;
		}
    }

	///////////////////	///////////////////

    SV_INTERNAL void show_reset_popup()
    {
		if (dev.engine_state == EngineState_ProjectManagement || dev.engine_state == EngineState_None) {
			return;
		}
	
		if (dev.engine_state != EngineState_ProjectManagement && show_dialog_yesno("Code reloaded!!", "Do you want to reset the game?")) {

			_engine_reset_game();
			dev.next_engine_state = EngineState_Edit;
		}
    }

    /////////////////////////////////////////////// KEY SHORTCUTS //////////////////////////////////////

    SV_AUX void update_key_shortcuts()
    {
		bool control = input.keys[Key_Control];
		bool alt = input.keys[Key_Alt];
		//bool shift = input.keys[Key_Shift];
			
		// Fullscreen
		if (input.keys[Key_F11] == InputState_Pressed) {
			
			os_window_set_fullscreen(os_window_state() != WindowState_Fullscreen);
		}

		if (alt && input.keys[Key_F4] == InputState_Pressed) {
			
			engine.close_request = true;
		}

		// Debug rendering
		if (dev.engine_state == EngineState_Play && input.keys[Key_F1] == InputState_Pressed) {
			editor.debug_draw = !editor.debug_draw;
		}

		// Console show - hide
		if (input.keys[Key_F3] == InputState_Pressed) {

			if (console_is_open())
				console_close();
			else console_open();
		}

		if (input.keys[Key_F2] == InputState_Pressed && editor.selected_entities.size()) {
			editor.entity_viewer = true;
		}

		// Change debug camera projection
		if (editor.show_editor && !alt && input.keys[Key_F4] == InputState_Pressed) {

			dev.camera.projection_type = (dev.camera.projection_type == ProjectionType_Orthographic) ? ProjectionType_Perspective : ProjectionType_Orthographic;

			if (dev.camera.projection_type == ProjectionType_Orthographic) {

				dev.camera.width = 30.f;
				dev.camera.height = 30.f;
				dev.camera.near = -1000.f;
				dev.camera.far = 1000.f;
				dev.camera.rotation = { 0.f, 0.f, 0.f, 1.f };
			}
			else {

				dev.camera.width = 0.1f;
				dev.camera.height = 0.1f;
				dev.camera.near = 0.03f;
				dev.camera.far = 3000.f;
			}
		}
	
		// Compile game code
		if (input.keys[Key_F5] == InputState_Pressed) {

			char path[FILEPATH_SIZE + 1u];
			string_copy(path, engine.project_path, FILEPATH_SIZE + 1u);
			
			CompilePluginDesc desc;
			desc.plugin_path = path;
			
			if (!compile_plugin(desc)) {
			}
		}
	
		if (editor.in_editor_view) {
	    
			if (input.unused && control) {

				// Undo
				if (input.keys[Key_Z] == InputState_Pressed) {

					DoUndoStack& stack = dev.do_undo_stack;
					stack.lock();
					stack.undo_action(nullptr);
					stack.unlock();
				}

				// Do
				if (input.keys[Key_Y] == InputState_Pressed) {

					DoUndoStack& stack = dev.do_undo_stack;
					stack.lock();
					stack.do_action(nullptr);
					stack.unlock();
				}

				// Duplicate
				if (input.keys[Key_D] == InputState_Pressed) {

					for (Entity e : editor.selected_entities) {
						duplicate_entity(e);
					}
				}
			}

			if (input.unused) {

				// Remove
				if (input.keys[Key_Supr] == InputState_Pressed) {

					for (Entity e : editor.selected_entities) {
						destroy_entity(e);
					}

					editor.selected_entities.reset();
				}
			}
		}
		else {
		}
    }

    /////////////////////////////////////////////// CAMERA ///////////////////////////////////////

    static void control_camera()
    {
		if (dev.postprocessing) {

			CameraComponent* cam = get_main_camera();

			if (cam) {
				dev.camera.bloom = cam->bloom;
				dev.camera.ssao = cam->ssao;
			}
			else dev.postprocessing = false;
		}
		else {
			dev.camera.bloom = {};
			dev.camera.ssao = {};
		}
	
		if (!input.unused)
			return;

		if (!editor.in_editor_view)
			return;

		if (editor.entity_viewer) {

			v3_f32 position;

			for (Entity e : editor.selected_entities) {

				position += get_entity_world_position(e);
			}
			position /= (f32)editor.selected_entities.size();

			if (dev.camera.projection_type == ProjectionType_Perspective) {
				
				dev.camera.position = position;
				dev.camera.position += v3_f32(0.f, 0.f, -3.f);
			}
			else if (dev.camera.projection_type == ProjectionType_Orthographic) {

				dev.camera.position = position;
			}
		}
		else {
			
			if (dev.camera.projection_type == ProjectionType_Perspective) {

				XMVECTOR rotation = vec4_to_dx(dev.camera.rotation);

				XMVECTOR direction;
				XMMATRIX rotation_matrix;

				// Rotation matrix
				rotation_matrix = XMMatrixRotationQuaternion(rotation);

				// Camera direction
				direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
				direction = XMVector3Transform(direction, rotation_matrix);

				// Zoom
				if (input.mouse_wheel != 0.f) {

					f32 force = dev.camera.velocity;
					if (input.keys[Key_Shift] == InputState_Hold)
						force *= 3.f;

					dev.camera.position += v3_f32(direction) * input.mouse_wheel * force;
					input.unused = false;
				}

				// Camera rotation
				if (input.mouse_buttons[MouseButton_Center] == InputState_Pressed) {

					editor.camera_focus = true;
				}
				else if (input.mouse_buttons[MouseButton_Center] == InputState_Released) {
					editor.camera_focus = false;
				}

				if (editor.camera_focus && (input.mouse_dragged.x != 0.f || input.mouse_dragged.y != 0.f)) {

					v2_f32 drag = editor.absolute_mouse_dragged * 3.f;

					dev.camera.pitch = SV_MIN(SV_MAX(dev.camera.pitch - drag.y, -PI/2), PI/2);
					dev.camera.yaw += drag.x;

					rotation = XMQuaternionRotationRollPitchYaw(dev.camera.pitch, dev.camera.yaw, 0.f);
					input.unused = false;
				}

				dev.camera.rotation = v4_f32(rotation);
			}
			else {

				InputState button_state = input.mouse_buttons[MouseButton_Center];

				if (button_state == InputState_Pressed) {
					editor.camera_focus = true;
				}
				else if (button_state == InputState_Released) {
					editor.camera_focus = false;
				}

				if (editor.camera_focus) {

					// TODO
					v2_f32 drag = editor.absolute_mouse_dragged / editor.editor_view_size;

					dev.camera.position -= vec2_to_vec3((drag * v2_f32{ dev.camera.width, dev.camera.height }));
					input.unused = false;
				}
				else editor.camera_focus = false;
			}
		}

		if (dev.camera.projection_type == ProjectionType_Orthographic) {

			if (input.mouse_wheel != 0.f) {

				f32 force = 0.05f;
				if (input.keys[Key_Shift] == InputState_Hold) force *= 3.f;

				f32 length = dev.camera.getProjectionLength();

				f32 new_length = length - input.mouse_wheel * length * force;
				dev.camera.setProjectionLength(new_length);

				input.unused = false;
			}
		}
    }

    /////////////////////////////////////////////// GIZMOS ///////////////////////////////////////////////////////

    SV_AUX v2_f32 world_point_to_screen(const XMMATRIX& vpm, const v3_f32& position)
    {
		XMVECTOR p = vec3_to_dx(position, 1.f);

		p = XMVector4Transform(p, vpm);
	
		f32 d = XMVectorGetW(p);
		p = XMVectorDivide(p, XMVectorSet(d, d, d, 1.f));
	
		return v2_f32(p);
    }

    SV_AUX v2_f32 project_line_onto_line(v2_f32 origin, v2_f32 d0, v2_f32 d1)
    {
		return origin + (vec2_dot(d1, d0) / vec2_dot(d1, d1)) * d1;
    }
    SV_AUX v3_f32 project_line_onto_line(v3_f32 origin, v3_f32 d0, v3_f32 d1)
    {
		return origin + (vec3_dot(d1, d0) / vec3_dot(d1, d1)) * d1;
    }

    SV_AUX void intersect_line_vs_plane(v3_f32 line_point, v3_f32 line_direction, v3_f32 plane_point, v3_f32 plane_normal, v3_f32& intersection, f32& distance)
    {
		distance = vec3_dot(plane_point - line_point, plane_normal) / vec3_dot(line_direction, plane_normal);
		intersection = line_point + distance * line_direction;
    }

    SV_AUX void closest_points_between_two_lines(v3_f32 l0_pos, v3_f32 l0_dir, v3_f32 l1_pos, v3_f32 l1_dir, v3_f32& l0_res, v3_f32& l1_res)
    {
		v3_f32 u = l0_dir;
		v3_f32 v = l1_dir;
		v3_f32 w = l0_pos - l1_pos;
		f32 a = vec3_dot(u, u);         // always >= 0
		f32 b = vec3_dot(u, v);
		f32 c = vec3_dot(v, v);         // always >= 0
		f32 d = vec3_dot(u, w);
		f32 e = vec3_dot(v, w);
		f32 D = a*c - b*b;        // always >= 0
		f32 sc, tc;

		constexpr f32 SMALL_NUM = 0.00000001f;
	
		// compute the line parameters of the two closest points
		if (D < SMALL_NUM) {          // the lines are almost parallel
			sc = 0.0;
			tc = (b>c ? d/b : e/c);    // use the largest denominator
		}
		else {
			sc = (b*e - c*d) / D;
			tc = (a*e - b*d) / D;
		}

		l0_res = l0_pos + sc * u;
		l1_res = l1_pos + tc * v;
    }

    SV_AUX f32 relative_scalar(f32 value, v3_f32 position)
    {
		if (dev.camera.projection_type == ProjectionType_Perspective) {
		
			XMVECTOR pos = vec3_to_dx(position, 1.f);
			pos = XMVector4Transform(pos, dev.camera.view_matrix);
	    
			f32 distance = XMVectorGetZ(pos);

			f32 w_near_distance = math_sqrt(dev.camera.near * dev.camera.near + dev.camera.width);
			f32 w_near_prop = value / w_near_distance;

			w_near_distance = math_sqrt(distance * distance + dev.camera.width);

			f32 h_near_distance = math_sqrt(dev.camera.near * dev.camera.near + dev.camera.height);
			f32 h_near_prop = value / h_near_distance;

			h_near_distance = math_sqrt(distance * distance + dev.camera.height);

			return (w_near_distance * w_near_prop + h_near_distance * h_near_prop) * 0.5f;
		}
		else {

			return value * dev.camera.getProjectionLength();
		}
    }

	SV_AUX v3_f32 compute_selected_entities_position()
	{
		v3_f32 position;
		
		f32 mult = 1.f / f32(editor.selected_entities.size());
		for (Entity e : editor.selected_entities) {

			position += get_entity_world_position(e) * mult;
		}

		return position;
	}

	SV_AUX v3_f32 compute_selected_entities_scale()
	{
		v3_f32 scale;
		
		f32 mult = 1.f / f32(editor.selected_entities.size());
		for (Entity e : editor.selected_entities) {

			scale += get_entity_world_scale(e) * mult;
		}

		return scale;
	}

    SV_INTERNAL void update_gizmos()
    {
		GizmosData& info = editor.tool_data.gizmos_data;

		v3_f32 position = compute_selected_entities_position();
		
		// Compute axis size and selection size
		{
			info.axis_size = relative_scalar(GIZMOS_SIZE, position);
			info.selection_size = relative_scalar(0.008f, position);
		}

		if (!input.unused || editor.selected_entities.empty()) {
			info.focus = false;
			return;
		}

		v3_f32 scale = compute_selected_entities_scale();

		if (!info.focus) {

			info.object = GizmosObject_None;
	    
			v2_f32 mouse_position = input.mouse_position * 2.f;

			switch (info.mode)
			{
				
			case GizmosTransformMode_Position:
			case GizmosTransformMode_Scale:
			{
				GizmosObject obj[] = {
					GizmosObject_AxisX,
					GizmosObject_AxisY,
					GizmosObject_AxisZ
				};
		
				if (dev.camera.projection_type == ProjectionType_Perspective) {

					Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);
		    
					v3_f32 axis[3u];
					axis[0] = v3_f32(info.axis_size, 0.f, 0.f);
					axis[1] = v3_f32(0.f, info.axis_size, 0.f);
					axis[2] = v3_f32(0.f, 0.f, info.axis_size);

					// TODO: Sort axis update by the distance to the camera
					foreach(i, 3u) {

						v3_f32 p0, p1;
						closest_points_between_two_lines(ray.origin, ray.direction, position, axis[i], p0, p1);

						f32 dist0 = vec3_length(p0 - p1);
			
						f32 dist1 = -1.f;

						switch(i) {
			    
						case 0:
							dist1 = p1.x - position.x;
							break;
			    
						case 1:
							dist1 = p1.y - position.y;
							break;
			    
						case 2:
							dist1 = p1.z - position.z;
							break;
						}

						if (dist0 < info.selection_size && dist1 > 0.f && dist1 <= info.axis_size) {
							info.object = obj[i];
							info.start_offset = p1 - position;
							info.start_scale = scale;
						}
					}
				}
				else {

					v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + vec3_to_vec2(dev.camera.position);
					v2_f32 to_mouse = mouse - vec3_to_vec2(position);
			
					foreach (i, 2u) {

						v2_f32 axis_direction = ((i == 0) ? v2_f32::right() : v2_f32::up()) * info.axis_size;

						v2_f32 projection = project_line_onto_line(vec3_to_vec2(position), to_mouse, axis_direction);
						f32 dist0 = vec2_length(mouse - projection);
						f32 dist1 = ((i == 0) ? (projection.x - position.x) : (projection.y - position.y));
			
						if (dist0 < info.selection_size && dist1 > 0.f && dist1 <= info.axis_size) {
							info.object = obj[i];

							if (i == 0) info.start_offset = { dist1, 0.f, 0.f };
							else info.start_offset = { 0.f, dist1, 0.f };
						}
					}
				}
			}
			break;

			}

			if (info.object != GizmosObject_None && input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

		
				info.focus = true;
				input.unused = false;
			}
		}

		else {

			input.unused = false;

			if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

				info.focus = false;
			}
			else {

				v3_f32 init_pos = position;
				v3_f32 init_scale = scale;

				if (dev.camera.projection_type == ProjectionType_Perspective) {

					Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);

					v3_f32 axis;
		    
					switch (info.object) {

					case GizmosObject_AxisX:
						axis = v3_f32::right();
						break;

					case GizmosObject_AxisY:
						axis = v3_f32::up();
						break;

					case GizmosObject_AxisZ:
						axis = v3_f32::forward();
						break;
		    
					}

					v3_f32 p0, p1;
					closest_points_between_two_lines(ray.origin, ray.direction, position, axis, p0, p1);

					if (info.mode == GizmosTransformMode_Position) {

						v3_f32 move = p1 - info.start_offset;

						if (input.keys[Key_Shift]) {

							move.x = i32(move.x / info.position_advance) * info.position_advance;
							move.y = i32(move.y / info.position_advance) * info.position_advance;
							move.z = i32(move.z / info.position_advance) * info.position_advance;
						}

					
						position = move;
					}
					else if (info.mode == GizmosTransformMode_Scale) {
						
						scale = info.start_scale + (p1 - info.start_offset) - position;

						switch (info.object) {

						case GizmosObject_AxisX:
							scale.y = init_scale.y;
							scale.z = init_scale.z;
							break;

						case GizmosObject_AxisY:
							scale.x = init_scale.x;
							scale.z = init_scale.z;
							break;

						case GizmosObject_AxisZ:
							scale.x = init_scale.x;
							scale.y = init_scale.y;
							break;
							
						}
					}
				}
				else {

					v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + vec3_to_vec2(dev.camera.position);

					v2_f32 move = mouse - vec3_to_vec2(info.start_offset);

					if (input.keys[Key_Shift]) {

						move.x = i32(move.x / info.position_advance) * info.position_advance;
						move.y = i32(move.y / info.position_advance) * info.position_advance;
					}
		
					if (info.object == GizmosObject_AxisX)
						position.x = move.x;
					else if (info.object == GizmosObject_AxisY)
						position.y = move.y;
				}

				// Set new position
				v3_f32 move_position = position - init_pos;
				v3_f32 move_scale = scale - init_scale;
				
				for (Entity e : editor.selected_entities) {

					v3_f32& entity_position = *get_entity_position_ptr(e);
					v3_f32& entity_scale = *get_entity_scale_ptr(e);

					Entity parent = get_entity_parent(e);

					if (entity_exists(parent)) {

						XMMATRIX wm = get_entity_world_matrix(parent);

						wm = XMMatrixInverse(NULL, wm);
						
						entity_position += (v3_f32)XMVector4Transform(vec3_to_dx(move_position), wm);
						entity_scale += (v3_f32)XMVector4Transform(vec3_to_dx(move_scale), wm);
					}
					else {
						entity_position += move_position;
						entity_scale += move_scale;
					}
				}
			}
		}
    }

    SV_INTERNAL void draw_gizmos(GPUImage* offscreen, CommandList cmd)
    {
		if (editor.selected_entities.empty()) return;

		GizmosData& info = editor.tool_data.gizmos_data;

		v3_f32 position = compute_selected_entities_position();

		f32 axis_size = info.axis_size;

		switch (info.mode)
		{

		case GizmosTransformMode_Position:
		{
			Color color = ((info.object == GizmosObject_AxisX) ? (info.focus ? Color::Silver() : Color{255u, 50u, 50u, 255u}) : Color::Red());
			imrend_draw_line(position, position + v3_f32::right() * axis_size, color, cmd);

			color = ((info.object == GizmosObject_AxisY) ? (info.focus ? Color::Silver() : Color::Lime()) : Color::Green());
			imrend_draw_line(position, position + v3_f32::up() * axis_size, color, cmd);

			color = ((info.object == GizmosObject_AxisZ) ? (info.focus ? Color::Silver() : Color{50u, 50u, 255u, 255u}) : Color::Blue());
			imrend_draw_line(position, position + v3_f32::forward() * axis_size, color, cmd);
		}
		break;

		case GizmosTransformMode_Scale:
		{
			Color color = ((info.object == GizmosObject_AxisX) ? (info.focus ? Color::Silver() : Color{255u, 50u, 50u, 255u}) : Color::Red());
			imrend_draw_line(position, position + v3_f32::right() * axis_size, color, cmd);
			imrend_draw_quad(position + v3_f32::right() * axis_size, { axis_size * 0.1f }, color, cmd);

			color = ((info.object == GizmosObject_AxisY) ? (info.focus ? Color::Silver() : Color::Lime()) : Color::Green());
			imrend_draw_line(position, position + v3_f32::up() * axis_size, color, cmd);
			imrend_draw_quad(position + v3_f32::up() * axis_size, { axis_size * 0.1f }, color, cmd);

			color = ((info.object == GizmosObject_AxisZ) ? (info.focus ? Color::Silver() : Color{50u, 50u, 255u, 255u}) : Color::Blue());
			imrend_draw_line(position, position + v3_f32::forward() * axis_size, color, cmd);
			imrend_draw_quad(position + v3_f32::forward() * axis_size, { axis_size * 0.1f }, color, cmd);
		}
		break;

		}
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    bool _editor_initialize()
    {		
		SV_CHECK(_gui_initialize());

		load_asset_from_file(editor.image, "$system/images/editor.png");

		event_register("user_callbacks_initialize", show_reset_popup, 0u);

		// Entity Hierarchy
		{
			event_register("on_entity_create", create_entity_element, 0u);
			event_register("on_entity_destroy", destroy_entity_element, 0u);

			event_register("pre_initialize_scene", load_hierarchy_state, 0u);
			event_register("initialize_scene", validate_hierarchy, 0u);
			event_register("close_scene", save_hierarchy_state, 0u);

			auto& data = editor.entity_hierarchy_data;
			data.root.type = HierarchyElementType_Root;
			data.root.entity = 0;
			data.root.folder_name[0] = '\0';
			data.folder_id_count = 0;
		}

		dev.engine_state = EngineState_ProjectManagement;
		_gui_load("PROJECT");
		engine.update_scene = false;

		// Create offscreen
		GPUImageDesc desc;
		desc.format = OFFSCREEN_FORMAT;
		desc.layout = GPUImageLayout_ShaderResource;
		desc.type = GPUImageType_ShaderResource;
		// TODO
		desc.width = 1920u;
		desc.height = 1080u;

		SV_CHECK(graphics_image_create(&desc, &editor.offscreen_editor));
		SV_CHECK(graphics_image_create(&desc, &editor.offscreen_game));

		return true;
    }

    bool _editor_close()
    {
		SV_CHECK(_gui_close());

		unload_asset(editor.image);

		graphics_destroy(editor.offscreen_editor);
		graphics_destroy(editor.offscreen_game);
		
		return true;
    }
    
    SV_INTERNAL bool show_component_info(CompID comp_id, Component* comp)
    {
		bool remove;

		if (egui_begin_component(comp_id, &remove)) {

			if (get_component_id("Sprite") == comp_id) {

				SpriteComponent& spr = *reinterpret_cast<SpriteComponent*>(comp);

				gui_sprite("Sprite", spr.sprite_sheet, spr.sprite_id);
				gui_drag_color("Color", spr.color);
				gui_drag_color("Emissive Color", spr.emissive_color);
				gui_drag_u32("Layer", spr.layer, 1u, 0u, RENDER_LAYER_COUNT);

				bool xflip = spr.flags & SpriteComponentFlag_XFlip;
				bool yflip = spr.flags & SpriteComponentFlag_YFlip;

				if (gui_checkbox("XFlip", xflip, 3u)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
				if (gui_checkbox("YFlip", yflip, 4u)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
			}

			if (get_component_id("Textured Sprite") == comp_id) {

				TexturedSpriteComponent& spr = *reinterpret_cast<TexturedSpriteComponent*>(comp);

				gui_texture_asset("Texture", spr.texture);
				gui_drag_v4_f32("Texcoord", spr.texcoord, 0.01f);
				gui_drag_color("Color", spr.color, 1u);
				gui_drag_u32("Layer", spr.layer, 1u, 0u, RENDER_LAYER_COUNT);

				bool xflip = spr.flags & SpriteComponentFlag_XFlip;
				bool yflip = spr.flags & SpriteComponentFlag_YFlip;

				if (gui_checkbox("XFlip", xflip, 3u)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
				if (gui_checkbox("YFlip", yflip, 4u)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
			}

			if (get_component_id("Animated Sprite") == comp_id) {

				AnimatedSpriteComponent& spr = *reinterpret_cast<AnimatedSpriteComponent*>(comp);

				SpriteSheet* sheet = spr.sprite_sheet.get();
				u32 max_index = 0u;

				if (sheet && sheet->sprite_animations.exists(spr.animation_id))
					max_index = sheet->sprite_animations[spr.animation_id].frames;

				gui_sprite_animation("Animation", spr.sprite_sheet, spr.animation_id, 0u);
				gui_drag_color("Color", spr.color, 1u);
				gui_drag_u32("Index", spr.index, 1u, 0u, max_index, 2u);
				gui_drag_f32("Time Mult", spr.time_mult, 0.01f, 0.f, f32_max, 6u);

				bool xflip = spr.flags & SpriteComponentFlag_XFlip;
				bool yflip = spr.flags & SpriteComponentFlag_YFlip;

				if (gui_checkbox("XFlip", xflip, 9u)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
				if (gui_checkbox("YFlip", yflip, 10u)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
			}
	    
			if (get_component_id("Mesh") == comp_id) {

				MeshComponent& m = *reinterpret_cast<MeshComponent*>(comp);

				egui_comp_mesh("Mesh", 0u, &m.mesh);
				egui_comp_material("Material", 1u, &m.material);

				if (gui_button("Edit Material")) {

					editor.material_editor_data.material = m.material;
				}

				if (gui_button("Set cube")) {

					create_asset_from_name(m.mesh, "Mesh", "Cube");
				}
				if (gui_button("Set shpere")) {

					create_asset_from_name(m.mesh, "Mesh", "Sphere");
				}
				/*if (m.material.get())
				  gui_material(*m.material.get());*/
			}

			if (get_component_id("Camera") == comp_id) {

				CameraComponent& cam = *reinterpret_cast<CameraComponent*>(comp);

				f32 near_min;
				f32 near_max;
				f32 near_adv;
				f32 far_min;
				f32 far_max;
				f32 far_adv;

				if (cam.projection_type == ProjectionType_Perspective) {
					near_min = 0.001f;
					near_max = f32_max;
					near_adv = 0.01f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
				}
				else {
					near_min = f32_min;
					near_max = cam.far;
					near_adv = 0.3f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
				}

				gui_checkbox("Adjust Width", cam.adjust_width);

				bool perspective = cam.projection_type == ProjectionType_Perspective;

				if (gui_checkbox("Perspective", perspective)) {

					if (perspective) {
						cam.projection_type = ProjectionType_Perspective;
						cam.near = 0.2f;
						cam.far = 10000.f;
						cam.width = 0.9f;
						cam.height = 0.9f;
					}
					else {
						cam.projection_type = ProjectionType_Orthographic;
						cam.near = -1000.f;
						cam.far = 1000.f;
						cam.width = 100.f;
						cam.height = 100.f;
					}
				}

				gui_drag_f32("Near", cam.near, near_adv, near_min, near_max);
				gui_drag_f32("Far", cam.far, far_adv, far_min, far_max);
				gui_drag_f32(cam.adjust_width ? "Height" : "Width", cam.adjust_width ? cam.height : cam.width, 0.01f, 0.01f, f32_max);

				gui_checkbox("Bloom", cam.bloom.active);
				if (cam.bloom.active) {

					gui_drag_f32("Threshold", cam.bloom.threshold, 0.001f, 0.f, 10.f);
					gui_drag_f32("Intensity", cam.bloom.intensity, 0.001f, 0.f, 1.f);
					gui_drag_f32("Strength", cam.bloom.strength, 0.001f, 0.f, f32_max);
					gui_drag_u32("Iterations", cam.bloom.iterations, 1u, 1u, 100u);
				}

				gui_checkbox("SSAO", cam.ssao.active);
				if (cam.ssao.active) {

					gui_drag_u32("Samples", cam.ssao.samples, 1u, 5, 128);
					gui_drag_f32("Radius", cam.ssao.radius, 0.001f, 0.001f, 100.f);
					gui_drag_f32("Bias", cam.ssao.bias, 0.0001f, 0.f, 10.f);
				}
			}

			if (get_component_id("Light") == comp_id) {

				LightComponent& l = *reinterpret_cast<LightComponent*>(comp);

				const char* preview = "None";

				switch (l.light_type) {

				case LightType_Point:
					preview = "Point";
					break;

				case LightType_Direction:
					preview = "Direction";
					break;
					
				}

				if (gui_begin_combobox(preview, 32544)) {

					if (l.light_type != LightType_Point && gui_button("Point")) {
						l.light_type = LightType_Point;
					}

					if (l.light_type != LightType_Direction && gui_button("Direction")) {
						l.light_type = LightType_Direction;
					}
					
					gui_end_combobox();
				}

				gui_drag_color("Color", l.color);
				gui_drag_f32("Intensity", l.intensity, 0.05f, 0.0f, f32_max);

				if (l.light_type == LightType_Point) {
					
					gui_drag_f32("Range", l.range, 0.1f, 0.0f, f32_max);
					gui_drag_f32("Smoothness", l.smoothness, 0.005f, 0.0f, 1.f);
				}

				gui_separator(1);

				gui_checkbox("Shadow Mapping", l.shadow_mapping_enabled);

				if (l.shadow_mapping_enabled && l.light_type == LightType_Direction) {

					gui_drag_f32("Shadow Bias", l.shadow_bias, 0.01f, 0.f, f32_max);

					gui_drag_f32("Cascade 0", l.cascade_distance[0], 0.1f, 0.f, f32_max);
					gui_drag_f32("Cascade 1", l.cascade_distance[1], 0.1f, 0.f, f32_max);
					gui_drag_f32("Cascade 2", l.cascade_distance[2], 0.1f, 0.f, f32_max);
				}
			}

			if (get_component_id("Audio Source") == comp_id) {

				AudioSourceComponent& s = *reinterpret_cast<AudioSourceComponent*>(comp);

				Sound sound;
				u32 loop_count;
				audio_sound_get(s.source, sound, loop_count);
				
				if (gui_sound_asset("Sound", sound)) {
					audio_sound_set(s.source, sound, SV_MAX(loop_count, 1u));
				}

				if (gui_button("Play")) {
					s.play();
				}
				if (gui_button("Stop")) {
					s.stop();
				}
			}

			if (get_component_id("Body") == comp_id) {

				BodyComponent& body = *(BodyComponent*)comp;

				bool dynamic = body_type_get(body) == BodyType_Dynamic;

				if (gui_checkbox("Dynamic", dynamic)) {
					if (dynamic) body_type_set(body, BodyType_Dynamic);
					else body_type_set(body, BodyType_Static);
				}

				if (dynamic) {

					f32 linear_damping = body_linear_damping_get(body);
					f32 angular_damping = body_angular_damping_get(body);
					v3_f32 velocity = body_velocity_get(body);
					v3_f32 angular_velocity = body_angular_velocity_get(body);
					f32 mass = body_mass_get(body);

					if (gui_drag_f32("Linear Damping", linear_damping, 0.01f, 0.f, f32_max)) {
						body_linear_damping_set(body, linear_damping);
					}
					if (gui_drag_f32("Angular Damping", angular_damping)) {
						body_angular_damping_set(body, angular_damping);
					}
					if (gui_drag_f32("Mass", mass)) {
						body_mass_set(body, mass);
					}
					if (gui_drag_v3_f32("Velocity", velocity)) {
						body_velocity_set(body, velocity);
					}
					if (gui_drag_v3_f32("Angular Velocity", angular_velocity)) {
						body_angular_velocity_set(body, angular_velocity);
					}

					gui_separator(1);

					bool lock_position_x = body_is_locked(body, BodyLock_PositionX);
					bool lock_position_y = body_is_locked(body, BodyLock_PositionY);
					bool lock_position_z = body_is_locked(body, BodyLock_PositionZ);

					bool lock_rotation_x = body_is_locked(body, BodyLock_RotationX);
					bool lock_rotation_y = body_is_locked(body, BodyLock_RotationY);
					bool lock_rotation_z = body_is_locked(body, BodyLock_RotationZ);

					if (gui_checkbox("Position X", lock_position_x)) {
						if (lock_position_x) body_lock(body, BodyLock_PositionX);
						else body_unlock(body, BodyLock_PositionX);
					}
					if (gui_checkbox("Position Y", lock_position_y)) {
						if (lock_position_y) body_lock(body, BodyLock_PositionY);
						else body_unlock(body, BodyLock_PositionY);
					}
					if (gui_checkbox("Position Z", lock_position_z)) {
						if (lock_position_z) body_lock(body, BodyLock_PositionZ);
						else body_unlock(body, BodyLock_PositionZ);
					}
					if (gui_checkbox("Rotation X", lock_rotation_x)) {
						if (lock_rotation_x) body_lock(body, BodyLock_RotationX);
						else body_unlock(body, BodyLock_RotationX);
					}
					if (gui_checkbox("Rotation Y", lock_rotation_y)) {
						if (lock_rotation_y) body_lock(body, BodyLock_RotationY);
						else body_unlock(body, BodyLock_RotationY);
					}
					if (gui_checkbox("Rotation Z", lock_rotation_z)) {
						if (lock_rotation_z) body_lock(body, BodyLock_RotationZ);
						else body_unlock(body, BodyLock_RotationZ);
					}
				}
			}

			if (get_component_id("Box Collider") == comp_id) {

				BoxCollider& box = *(BoxCollider*)comp;

				gui_drag_v3_f32("Size", box.size, 0.01f, 0.00001f, f32_max);
			}

			if (get_component_id("Sphere Collider") == comp_id) {

				SphereCollider& sphere = *(SphereCollider*)comp;

				gui_drag_f32("Radius", sphere.radius, 0.01f, 0.00001f, f32_max);
			}

			DisplayComponentEvent e;
			e.comp_id = comp_id;
			e.comp = comp;

			gui_push_id("User");
			event_dispatch("display_component_data", &e);
			gui_pop_id();

			egui_end_component();
		}

		return remove;
    }

    SV_AUX void select_entity()
    {
		v2_f32 mouse = input.mouse_position;

		Ray ray = screen_to_world_ray(mouse, dev.camera.position, dev.camera.rotation, &dev.camera);

		XMVECTOR ray_origin = vec3_to_dx(ray.origin, 1.f);
		XMVECTOR ray_direction = vec3_to_dx(ray.direction, 0.f);

		Entity selected = 0;
		f32 distance = f32_max;

		XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
		XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

		XMVECTOR v0;
		XMVECTOR v1;
		XMVECTOR v2;
		XMVECTOR v3;

		u32 light_id = get_component_id("Light");
		u32 mesh_id = get_component_id("Mesh");
		u32 sprite_id = get_component_id("Sprite");
		u32 textured_sprite_id = get_component_id("Textured Sprite");
		u32 animated_sprite_id = get_component_id("Animated Sprite");
		
		// Select lights
		foreach_component(light_id, it, 0) {
			
			Entity entity = it.entity;
			
			if (is_entity_selected(entity))
				break;
			
			v3_f32 pos = get_entity_world_position(entity);
			
			f32 min_scale = relative_scalar(0.02f, pos);
			f32 scale = SV_MAX(min_scale, 1.f);
			
			XMMATRIX tm = XMMatrixScaling(scale, scale, 1.f) * XMMatrixRotationQuaternion(vec4_to_dx(dev.camera.rotation)) * XMMatrixTranslation(pos.x, pos.y, pos.z);
			
			v0 = XMVector3Transform(p0, tm);
			v1 = XMVector3Transform(p1, tm);
			v2 = XMVector3Transform(p2, tm);
			v3 = XMVector3Transform(p3, tm);
			
			f32 dis = f32_max;
			
			v3_f32 intersection;

			// TODO: Ray vs Quad intersection
			
			if (intersect_ray_vs_traingle(ray, v3_f32(v0), v3_f32(v1), v3_f32(v2), intersection)) {
				
				dis = vec3_length(intersection);
			}
			else if (intersect_ray_vs_traingle(ray, v3_f32(v1), v3_f32(v3), v3_f32(v2), intersection)) {
				
				dis = SV_MIN(vec3_length(intersection), dis);
			}
			
			if (dis < distance) {
				distance = dis;
				selected = entity;
			}
		}

		if (selected == 0) {

			// Select meshes
			foreach_component(mesh_id, it, 0) {
				
				Entity entity = it.entity;
				MeshComponent& m = *(MeshComponent*)it.comp;
				
				if (is_entity_selected(entity))
					break;
				
				if (m.mesh.get() == nullptr) break;

				XMMATRIX wm = get_entity_world_matrix(entity);

				XMMATRIX itm = XMMatrixInverse(0, wm);

				ray.origin = v3_f32(XMVector4Transform(ray_origin, itm));
				ray.direction = v3_f32(XMVector4Transform(ray_direction, itm));

				Mesh& mesh = *m.mesh.get();

				u32 triangles = u32(mesh.indices.size()) / 3u;

				for (u32 i = 0u; i < triangles; ++i) {

					u32 i0 = mesh.indices[i * 3u + 0u];
					u32 i1 = mesh.indices[i * 3u + 1u];
					u32 i2 = mesh.indices[i * 3u + 2u];

					v3_f32 p0 = mesh.positions[i0];
					v3_f32 p1 = mesh.positions[i1];
					v3_f32 p2 = mesh.positions[i2];

					v3_f32 intersection;

					if (intersect_ray_vs_traingle(ray, p0, p1, p2, intersection)) {

						intersection = XMVector4Transform(vec3_to_dx(intersection, 1.f), wm);

						f32 dis = vec3_distance(intersection, ray_origin);
						if (dis < distance) {
							distance = dis;
							selected = entity;
						}

					}
				}
			}

			// Select sprites
			{
				ray.origin = v3_f32(ray_origin);
				ray.direction = v3_f32(ray_direction);

				auto sprite_intersect = [&](Entity entity) {
					
					if (is_entity_selected(entity))
						return;

					XMMATRIX tm = get_entity_world_matrix(entity);

					v0 = XMVector3Transform(p0, tm);
					v1 = XMVector3Transform(p1, tm);
					v2 = XMVector3Transform(p2, tm);
					v3 = XMVector3Transform(p3, tm);

					f32 dis = f32_max;

					v3_f32 intersection;

					// TODO: Ray vs Quad intersection

					if (intersect_ray_vs_traingle(ray, v3_f32(v0), v3_f32(v1), v3_f32(v2), intersection)) {

						dis = vec3_length(intersection);
					}
					else if (intersect_ray_vs_traingle(ray, v3_f32(v1), v3_f32(v3), v3_f32(v2), intersection)) {
			
						dis = SV_MIN(vec3_length(intersection), dis);
					}

					if (dis < distance) {
						distance = dis;
						selected = entity;
					}
				};

				foreach_component(sprite_id, it, 0)
					sprite_intersect(it.entity);
				
				foreach_component(textured_sprite_id, it, 0)
					sprite_intersect(it.entity);
				
				foreach_component(animated_sprite_id, it, 0)
					sprite_intersect(it.entity);
			}
		}
		
		select_entity(selected, !input.keys[Key_Shift]);
	}

    void display_entity_inspector()
    {
		Entity selected = (editor.selected_entities.size() == 1u) ? editor.selected_entities.back() : 0;

		if (gui_begin_window("Create Tag", GuiWindowFlag_Temporal)) {

			auto& data = editor.create_tag_data;
			
			gui_text_field(data.name, TAG_NAME_SIZE + 1u, 73982);

			if (data.name[0]) {
				
				if (gui_button("Create")) {
				
					create_tag(data.name);

					gui_hide_window("Create Tag");
					data.name[0] = '\0';
				}
			}
			
			gui_end_window();
		}

		if (gui_begin_window("Inspector")) {

			if (selected != 0) {

				// Entity name
				{
					char entity_name[ENTITY_NAME_SIZE + 1];
					string_copy(entity_name, get_entity_name(selected), ENTITY_NAME_SIZE + 1);;
					
					if (gui_text_field(entity_name, ENTITY_NAME_SIZE + 1, 32498234)) {
						set_entity_name(selected, entity_name);
					}
				}

				// Entity transform
				egui_transform(selected);

				// Entity components
				{
					u32 comp_count = get_entity_component_count(selected);

					gui_push_id("Entity Components");

					foreach(comp_index, comp_count) {

						CompRef ref = get_entity_component_by_index(selected, comp_index);

						if (show_component_info(ref.comp_id, ref.comp)) {

							remove_entity_component(selected, ref.comp_id);
							comp_count = get_entity_component_count(selected);
						}
					}

					gui_pop_id();
				}

				gui_separator(3);
				// Entity Tags
				{
					if (gui_collapse("Tags")) {

						foreach(tag, TAG_MAX) {

							if (has_entity_tag(selected, tag)) {

								if (gui_button(get_tag_name(tag)))
									remove_entity_tag(selected, tag);
							}
						}

						gui_separator(1);

						gui_button("Add Tag");
						
						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							foreach(tag, TAG_MAX) {

								if (tag_exists(tag) && !has_entity_tag(selected, tag)) {

									if (gui_button(get_tag_name(tag))) {

										add_entity_tag(selected, tag);
										gui_close_popup();
									}
								}
							}

							gui_separator(1);

							if (gui_button("Create Tag")) {
								gui_show_window("Create Tag");
								gui_close_popup();
							}
									
							gui_end_popup();
						}
					}
				}

				// Entity Info
				gui_separator(3);
				
				if (gui_collapse("Entity Data")) {
					
					gui_push_id("Entity Data");

					SceneData* scene = get_scene_data();

					{
						bool main = scene->main_camera == selected;

						if (gui_checkbox("Main Camera", main)) {

							if (main) scene->main_camera = selected;
							else scene->main_camera = 0;
						}
					}

					gui_push_id("User");

					DisplayEntityEvent event;
					event.entity = selected;

					event_dispatch("display_entity_data", &event);
		    
					gui_pop_id(2u);
				}

				if (gui_begin_popup(GuiPopupTrigger_Root)) {
		    
					u32 count = get_component_register_count();
					foreach(i, count) {

						CompID comp_id = CompID(i);

						if (has_entity_component(selected, comp_id))
							continue;
			
						if (gui_button(get_component_name(comp_id), comp_id)) {
			    
							add_entity_component(selected, comp_id);
						}
					}

					gui_end_popup();
				}
			}

			gui_end_window();
		}
    }

    SV_INTERNAL void display_asset_browser()
    {
		AssetBrowserInfo& info = editor.asset_browser;
		
		bool update_browser = false;
		char next_filepath[FILEPATH_SIZE + 1] = "";

		string_copy(next_filepath, info.filepath, FILEPATH_SIZE + 1u);

		if (gui_begin_window("Create Folder", GuiWindowFlag_Temporal)) {

			static char foldername[FILENAME_SIZE + 1u] = "";
			
			gui_text_field(foldername, FILENAME_SIZE + 1u, 0u);

			if (gui_button("Create")) {
				
				update_browser = true;

				char filepath[FILEPATH_SIZE + 1u] = "assets/";
				string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
				string_append(filepath, foldername, FILEPATH_SIZE + 1u);

				if (!folder_create(filepath)) {
					SV_LOG_ERROR("Can't create the folder '%s'", filepath);
				}

				gui_hide_window("Create Folder");
				string_copy(foldername, "", FILENAME_SIZE + 1u);
			}

			gui_end_window();
		}

		if (gui_begin_window("Create Asset", GuiWindowFlag_Temporal)) {

			auto& data = info.create_asset_data;

			switch (data.state) {

			case 0:
				if (gui_button("Create SpriteSheet")) {
					data.state = 2u;
				}

				if (gui_button("Create Material")) {
					data.state = 1u;
				}
				break;

				// Create Material
			case 1:
			{
				gui_text_field(data.assetname, FILENAME_SIZE + 1u, 0u);

				if (gui_button("Create")) {

					Material mat;

					update_browser = true;

					char filepath[FILEPATH_SIZE + 1u] = "assets/";
					string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
					string_append(filepath, data.assetname, FILEPATH_SIZE + 1u);

					const char* extension = filepath_extension(data.assetname);
					extension = extension ? extension : "";
					
					if (!string_equals(extension, ".mat")) {
						string_append(filepath, ".mat", FILEPATH_SIZE + 1u);
					}
					
					save_material(mat, filepath);

					gui_hide_window("Create Asset");
				}
			}
			break;
			
			// Create Sprite Sheet
			case 2:
			{
				
			}
			break;
			
			}
			
			gui_end_window();
		}

		if (gui_begin_window("Import Model", GuiWindowFlag_Temporal)) {

			auto& data = editor.import_model_data;
			
			gui_separator(1);

			if (gui_collapse("Meshes")) {

				gui_push_id("Meshes");

				foreach(i, data.model_info.meshes.size()) {

					MeshInfo& mesh = data.model_info.meshes[i];
					gui_checkbox(mesh.name.c_str(), mesh.import, i);
				}

				gui_pop_id();
			}

			if (gui_collapse("Materials")) {

				gui_push_id("Materials");

				foreach(i, data.model_info.materials.size()) {

					MaterialInfo& mat = data.model_info.materials[i];
					gui_checkbox(mat.name.c_str(), mat.import, i);
				}

				gui_pop_id();
			}

			if (gui_button("Import")) {

				char dst[FILEPATH_SIZE + 1u];
				string_copy(dst, "assets/", FILEPATH_SIZE + 1u);
				string_append(dst, info.filepath, FILEPATH_SIZE + 1u);
				
				if (import_model(dst, data.model_info)) {
					SV_LOG_INFO("Model imported at '%s'", dst);
				}
				else SV_LOG_ERROR("Can't import the model at '%s'", dst);
				
				gui_hide_window("Import Model");
				data = {};
			}
			
			gui_end_window();
		}

		if (gui_begin_window("Asset Browser")) {

			// TEMP
			if (input.unused && input.keys[Key_Control] && input.keys[Key_B] == InputState_Pressed) {

				string_copy(next_filepath, info.filepath, FILEPATH_SIZE + 1u);

				char* end = next_filepath;
				char* it = next_filepath + string_size(next_filepath) - 1u;

				while (it > end && *it != '/') --it;

				if (it >= end) next_filepath[0] = '\0';
				else if (*it == '/') *(it + 1u) = '\0';
		
		
				update_browser = true;
			}

			if (gui_select_filepath(info.filepath, next_filepath, 0u)) {

				update_browser = true;
			}
	    
			{
				gui_begin_grid(85.f, 10.f, 1u);

				foreach(i, info.elements.size()) {

					const AssetElement& e = info.elements[i];

					gui_push_id((u64)e.type);

					// TODO: ignore unused elements
					switch (e.type) {

					case AssetElementType_Directory:
					{
						if (gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FOLDER, 0u)) {
			    
							if (e.type == AssetElementType_Directory && !update_browser) {

								update_browser = true;

								size_t new_size = strlen(info.filepath) + strlen(e.name) + 1u;
								if (new_size < FILEPATH_SIZE)
									sprintf(next_filepath, "%s%s/", info.filepath, e.name);
								else
									SV_LOG_ERROR("This filepath exceeds the max filepath size");
							}
						}

						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								update_browser = true;

								char filepath[FILEPATH_SIZE + 1u] = "assets/";
								string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
								string_append(filepath, e.name, FILEPATH_SIZE + 1u);

								if (folder_remove(filepath)) {
									SV_LOG_INFO("Folder '%s' removed", filepath);
								}
								else {
									SV_LOG_ERROR("Can't remove the folder '%s'", filepath);
								}
							}
							
							gui_end_popup();
						}
					}
					break;

					case AssetElementType_Prefab:
					{
						PrefabPackage pack;

						sprintf(pack.filepath, "assets/%s%s", info.filepath, e.name);
						
						gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FILE, 0u);
						gui_send_package(&pack, sizeof(PrefabPackage), ASSET_BROWSER_PREFAB);
					}
					break;

					case AssetElementType_Entity:
					{
						EntityPackage pack;

						sprintf(pack.filepath, "assets/%s%s", info.filepath, e.name);
						
						gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FILE, 0u);
						gui_send_package(&pack, sizeof(EntityPackage), ASSET_BROWSER_ENTITY);
					}
					break;

					case AssetElementType_Texture:
					case AssetElementType_Mesh:
					case AssetElementType_Material:
					case AssetElementType_SpriteSheet:
					case AssetElementType_Sound:
					{
						AssetPackage pack;

						sprintf(pack.filepath, "assets/%s%s", info.filepath, e.name);

						if (e.type == AssetElementType_Texture) {

							TextureAsset tex;

							if (load_asset_from_file(tex, pack.filepath, AssetLoadingPriority_GetIfExists)) {

								gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FILE, 0u);
							}
							// TODO: Set default image
							else {
								gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FILE, 0u);
							}
						}
						else {

							gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FILE, 0u);
						}

						u32 id;

						switch (e.type) {

						case AssetElementType_Texture:
							id = ASSET_BROWSER_PACKAGE_TEXTURE;
							break;

						case AssetElementType_Mesh:
							id = ASSET_BROWSER_PACKAGE_MESH;
							break;

						case AssetElementType_Material:
							id = ASSET_BROWSER_PACKAGE_MATERIAL;
							break;

						case AssetElementType_SpriteSheet:
							id = ASSET_BROWSER_PACKAGE_SPRITE_SHEET;
							break;

						case AssetElementType_Sound:
							id = ASSET_BROWSER_PACKAGE_SOUND;
							break;

						default:
							id = u32_max;
							break;
						}

						if (id != u32_max) {
			    
							gui_send_package(&pack, sizeof(AssetPackage), id);
						}

						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove")) {
								
							}

							if (id == ASSET_BROWSER_PACKAGE_SPRITE_SHEET) {

								if (gui_button("Edit")) {

									char filepath[FILEPATH_SIZE + 1u];
									string_copy(filepath, "assets/", FILEPATH_SIZE + 1u);
									string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
									string_append(filepath, e.name, FILEPATH_SIZE + 1u);
									
									edit_sprite_sheet(filepath);
								}
							}
							
							gui_end_popup();
						}
					}
					break;

					}
				}

				gui_end_grid();
			}

			// Update per time
			if (!update_browser) {
				f64 now = timer_now();

				if (now - info.last_update > 1.0) {
					update_browser = true;
					sprintf(next_filepath, "%s", info.filepath);
				}
			}

			// Update browser elements
			if (update_browser) {

				strcpy(info.filepath, next_filepath);
				sprintf(next_filepath, "assets/%s", info.filepath);
		
				while (!file_exists(next_filepath) && next_filepath[0]) {

					size_t size = strlen(next_filepath) - 1u;
		    
					next_filepath[size] = '\0';
		    
					while (size && next_filepath[size] != '/') {
						--size;
						next_filepath[size] = '\0';
					}
				}

				if (strlen(next_filepath) < strlen("assets/")) {
					strcpy(next_filepath, "assets/");
				}
		
				// Clear browser data
				info.elements.clear();

				FolderIterator it;
				FolderElement e;
		
				bool res = folder_iterator_begin(next_filepath, &it, &e);

				if (res) {

					do {

						if (strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0)
							continue;
			
						AssetElement element;
			
						// Select element type
						if (e.is_file) {

							if (strcmp(e.extension, "mesh") == 0) element.type = AssetElementType_Mesh;
							else if (strcmp(e.extension, "mat") == 0) element.type = AssetElementType_Material;
							else if (strcmp(e.extension, "png") == 0 || strcmp(e.extension, "jpg") == 0 || strcmp(e.extension, "gif") == 0 || strcmp(e.extension, "jpeg") == 0) element.type = AssetElementType_Texture;
							else if (strcmp(e.extension, "sprites") == 0) element.type = AssetElementType_SpriteSheet;
							else if (strcmp(e.extension, "wav") == 0) element.type = AssetElementType_Sound;
							else if (strcmp(e.extension, "prefab") == 0) element.type = AssetElementType_Prefab;
							else if (strcmp(e.extension, "entity") == 0) element.type = AssetElementType_Entity;
							else element.type = AssetElementType_Unknown;
						}
						else {
							element.type = AssetElementType_Directory;
						}

						// Set name
						size_t name_size = strlen(e.name);
						memcpy(element.name, e.name, name_size);
						element.name[name_size] = '\0';

						info.elements.emplace_back(element);
					}
					while (folder_iterator_next(&it, &e));
				}
				else {

					SV_LOG_ERROR("Can't create asset browser content at '%s'", next_filepath);
				}
		
		
				strcpy(info.filepath, next_filepath + strlen("assets/"));
				info.last_update = timer_now();
			}

			if (gui_begin_popup(GuiPopupTrigger_Root)) {

				u64 id = 0u;

				if (gui_button("Create Folder", id++)) {

					gui_show_window("Create Folder");
					gui_close_popup();
				}

				gui_separator(1);

				if (gui_button("Import Model")) {

					auto& data = editor.import_model_data;
					data.model_info = {};

					char filepath[FILEPATH_SIZE + 1u] = "";

					const char* filters[] = {
						"Wavefront OBJ (.obj)", "*.obj",
						"All", "*",
						""
					};
			
					if (file_dialog_open(filepath, 2u, filters, "")) {
			    
						if (load_model(filepath, data.model_info)) {

							gui_show_window("Import Model");
							gui_close_popup();
						}
						else {
							SV_LOG_ERROR("Can't load the model '%s'", filepath);
						}
					}
				}

				if (gui_button("Create Asset")) {

					auto& data = info.create_asset_data;

					data.state = 0u;
					gui_show_window("Create Asset");
					gui_close_popup();
				}

				if (gui_button("Create Entity Model")) {

					char filepath[FILEPATH_SIZE + 1u] = "assets/";
					string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);

					Entity parent = create_entity(0, filepath_name(filepath));

					if (parent) {

						if (create_entity_model(parent, filepath)) {
							SV_LOG_INFO("Entity model created: '%s'", filepath);
						}
						else {
							SV_LOG_ERROR("Can't create entity model: '%s'", filepath);
						}
					}
				}

				if (gui_button("Create Sprite Sheet")) {

					char filepath[FILEPATH_SIZE + 1u] = "assets/";

					string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
					string_append(filepath, "test.sprites", FILEPATH_SIZE + 1u);

					bool show_win = false;

					if (!file_exists(filepath)) {
					
						Serializer s;
						serialize_begin(s);

						serialize_sprite_sheet(s, {});

						if (!serialize_end(s, filepath)) {
							SV_LOG_ERROR("Can't create the sprite sheet at '%s'", filepath);
						}
						else {

							show_win = true;
						}
					}
					else show_win = true;

					if (show_win) {

						edit_sprite_sheet(filepath);
					}
				}
		
				gui_end_popup();
			}
	    
			gui_end_window();
		}
    }

    void display_scene_settings()
    {
		SceneData& s = *get_scene_data();

		if (gui_begin_window("Go to scene", GuiWindowFlag_Temporal)) {
		
			gui_text_field(editor.next_scene_name, SCENE_NAME_SIZE + 1u, 0u);

			gui_separator(1);

			if (gui_button("GO!")) {
				set_scene(editor.next_scene_name);
				string_copy(editor.next_scene_name, "", SCENE_NAME_SIZE + 1u);

				gui_hide_window("Go to scene");
			}

			gui_end_window();
		}
	
		if (gui_begin_window("Scene Settings")) {

			gui_text(get_scene_name(), 0u);

			if (gui_button("Go to scene"))
				gui_show_window("Go to scene");

			if (gui_collapse("Rendering")) {

				gui_drag_color("Ambient Light", s.ambient_light);

				// Skybox
				{
					gui_button("Skybox");

					AssetPackage* package;
					if (gui_recive_package((void**)&package, ASSET_BROWSER_PACKAGE_TEXTURE, GuiReciverTrigger_LastWidget)) {

						set_skybox(package->filepath);
					}
				}
			}

			if (gui_collapse("Physics")) {

				if (s.physics.in_3D) {
					gui_drag_v3_f32("Gravity", s.physics.gravity, 0.01f, -f32_max, f32_max);
				}
				else {
					v2_f32 v2;

					v2 = vec3_to_vec2(s.physics.gravity);
					if (gui_drag_v2_f32("Gravity", v2, 0.01f, -f32_max, f32_max))
						s.physics.gravity = vec2_to_vec3(v2);
				}
				
				gui_checkbox("3D", s.physics.in_3D);
			}
	    
			gui_end_window();
		}
    }

    SV_INTERNAL void display_spritesheet_editor()
    {
		if (gui_begin_window("SpriteSheet Editor")) {

			auto& data = editor.sprite_sheet_editor_data;

			SpriteSheet* sheet = data.current_sprite_sheet.get();

			if (sheet) {

				bool save = false;

				GPUImage* image = sheet->texture.get();

				// TODO: back button in the window
				
				switch (data.state) {

				case SpriteSheetEditorState_Main:
				{
					if (gui_button("Sprites", 0u)) {
						data.next_state = SpriteSheetEditorState_SpriteList;
					}
					if (gui_button("Sprite Animations", 1u)) {
						data.next_state = SpriteSheetEditorState_AnimationList;
					}
				}
				break;

				case SpriteSheetEditorState_SpriteList:
				{
					u64 id = 0u;

					if (gui_texture_asset("Texture", sheet->texture))
						save = true;
					
					if (gui_button("New Sprite", id++)) {
						data.next_state = SpriteSheetEditorState_NewSprite;
					}

					u32 remove_id = u32_max;

					GuiSpritePackage package;
					string_copy(package.sprite_sheet_filepath, data.current_sprite_sheet.get_filepath(), FILEPATH_SIZE + 1u);
					
					for (auto it = sheet->sprites.begin();
						 it.has_next();
						 ++it)
					{
						u32 index = it.get_index();
						Sprite& sprite = *it;

						gui_push_image(GuiImageType_Background, image, sprite.texcoord);
						
						if (gui_button(sprite.name, index + 28349u)) {

							data.modifying_id = index;
							data.next_state = SpriteSheetEditorState_ModifySprite;
						}

						gui_pop_image();

						package.sprite_id = it.get_index();
						gui_send_package(&package, sizeof(GuiSpritePackage), GUI_PACKAGE_SPRITE);
						
						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								remove_id = index;
							}

							gui_end_popup();
						}
					}

					if (remove_id != u32_max) {

						sheet->sprites.erase(remove_id);
						save = true;
					}
				}
				break;

				case SpriteSheetEditorState_NewSprite:
				{
					Sprite& sprite = data.temp_sprite;

					gui_push_image(GuiImageType_Background, image, sprite.texcoord);
					gui_image(80.f, 0u);
					gui_pop_image();

					gui_text_field(sprite.name, SPRITE_NAME_SIZE + 1u, 1u);

					gui_drag_v4_f32("Texcoord", sprite.texcoord, 0.001f, 0.f, 1.f, 2u);

					if (gui_button("Save", 3u)) {

						if (sheet->add_sprite(NULL, sprite.name, sprite.texcoord)) {
							
							save = true;
							data.next_state = SpriteSheetEditorState_SpriteList;
						}
					}
				}
				break;

				case SpriteSheetEditorState_ModifySprite:
				{
					if (!sheet->sprites.exists(data.modifying_id))
						data.next_state = SpriteSheetEditorState_SpriteList;
					else {
						
						Sprite& sprite = data.temp_sprite;

						gui_push_image(GuiImageType_Background, image, sprite.texcoord);
						gui_image(80.f, 0u);
						gui_pop_image();

						gui_text_field(sprite.name, SPRITE_NAME_SIZE + 1u, 1u);

						gui_drag_v4_f32("Texcoord", sprite.texcoord, 0.001f, 0.f, 1.f, 2u);

						if (gui_button("Save sprite", 3u)) {
							
							if (sheet->modify_sprite(data.modifying_id, sprite.name, sprite.texcoord)) {
								
								// TEMP
								data.next_state = SpriteSheetEditorState_SpriteList;

								save = true;
							}
						}
					}
				}
				break;

				case SpriteSheetEditorState_AnimationList:
				{
					u64 id = 0u;
					
					if (gui_button("New Sprite Animation", id++)) {
						data.next_state = SpriteSheetEditorState_NewAnimation;
					}

					u32 remove_id = u32_max;

					GuiSpriteAnimationPackage package;
					string_copy(package.sprite_sheet_filepath, data.current_sprite_sheet.get_filepath(), FILEPATH_SIZE + 1u);
					
					for (auto it = sheet->sprite_animations.begin();
						 it.has_next();
						 ++it)
					{
						u32 index = it.get_index();
						SpriteAnimation& anim = *it;

						u32 current_sprite = u32(data.simulation_time / anim.frame_time) % anim.frames;
						v4_f32 texcoord = sheet->get_sprite_texcoord(anim.sprites[current_sprite]);

						gui_push_image(GuiImageType_Background, image, texcoord);
						
						if (gui_button(anim.name, index + 28349u)) {

							data.modifying_id = index;
							data.next_state = SpriteSheetEditorState_ModifyAnimation;
						}

						gui_pop_image();

						package.animation_id = it.get_index();

						gui_send_package(&package, sizeof(GuiSpriteAnimationPackage), GUI_PACKAGE_SPRITE_ANIMATION);
						
						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								remove_id = index;
							}

							gui_end_popup();
						}
					}

					if (remove_id != u32_max) {

						sheet->sprite_animations.erase(remove_id);
						save = true;
					}

					data.simulation_time += engine.deltatime;
				}
				break;

				case SpriteSheetEditorState_NewAnimation:
				{
					SpriteAnimation& anim = data.temp_anim;

					if (anim.frames) {
						
						u32 current_spr = u32(data.simulation_time / anim.frame_time) % anim.frames;
						v4_f32 texcoord = sheet->get_sprite_texcoord(anim.sprites[current_spr]);

						gui_push_image(GuiImageType_Background, image, texcoord);
						gui_image(80.f, 0u);
						gui_pop_image();
					}
					else gui_image(80.f, 0u);

					gui_text_field(anim.name, SPRITE_NAME_SIZE + 1u, 1u);
					
					foreach(i, anim.frames) {

						u32 spr_id = anim.sprites[i];
						
						if (sheet->sprites.exists(spr_id)) {

							const Sprite& spr = sheet->sprites[spr_id];

							gui_push_image(GuiImageType_Background, image, spr.texcoord);

							if (gui_button(spr.name, spr_id + 84390u)) {
								
							}

							gui_pop_image();
						}
						else {
							// TODO
						}
					}

					if (gui_button("Add Sprite", 2u)) {
						data.next_state = SpriteSheetEditorState_AddSprite;
					}
					if (gui_button("Save", 3u)) {
						if (sheet->add_sprite_animation(NULL, anim.name, anim.sprites, anim.frames, anim.frame_time)) {
							
							save = true;
							data.next_state = SpriteSheetEditorState_AnimationList;
						}
					}

					data.simulation_time += engine.deltatime;
				}
				break;

				case SpriteSheetEditorState_AddSprite:
				{
					for (auto it = sheet->sprites.begin();
						 it.has_next();
						 ++it)
					{
						gui_push_image(GuiImageType_Background, image, it->texcoord);
						
						if (gui_button(it->name, it.get_index() + 38543u)) {

							data.next_state = SpriteSheetEditorState_NewAnimation;
							u32& spr = data.temp_anim.sprites[data.temp_anim.frames++];
							spr = it.get_index();
						}

						gui_pop_image();
					}
				}
				break;
					
				}

				// Initialize new state
				if (data.state != data.next_state) {

					switch (data.next_state) {

					case SpriteSheetEditorState_NewSprite:
					{
						string_copy(data.temp_sprite.name, "Name", SPRITE_NAME_SIZE + 1u);
						data.temp_sprite.texcoord = {0.f, 0.f, 1.f, 1.f};
					}
					break;

					case SpriteSheetEditorState_NewAnimation:
					{
						if (data.state != SpriteSheetEditorState_AddSprite) {
							
							string_copy(data.temp_anim.name, "Name", SPRITE_NAME_SIZE + 1u);
							data.temp_anim.frames = 0u;
							data.temp_anim.frame_time = 0.1f;
						}
					}
					break;

					case SpriteSheetEditorState_ModifySprite:
					{
						Sprite& s = sheet->sprites[data.modifying_id];
						string_copy(data.temp_sprite.name, s.name, SPRITE_NAME_SIZE + 1u);
						data.temp_sprite.texcoord = s.texcoord;
					}
					break;

					case SpriteSheetEditorState_AnimationList:
					{
						data.simulation_time = 0.f;
					}
					break;
						
					}

					data.state = data.next_state;
				}

				// Save SpriteSheet
				if (save) {

					Serializer s;
					serialize_begin(s);

					serialize_sprite_sheet(s, *sheet);

					serialize_end(s, data.current_sprite_sheet.get_filepath());
				}
			}
	    
			gui_end_window();
		}
		// Free asset
		else {
			unload_asset(editor.sprite_sheet_editor_data.current_sprite_sheet);
		}
    }

	SV_INTERNAL void display_material_editor()
	{
		if (gui_begin_window("Material Editor")) {

			auto& data = editor.material_editor_data;

			MaterialAsset& mat = data.material;

			if (mat.get()) {
	
				gui_text("Pipeline Settings");
				gui_separator(1);
			
				gui_checkbox("Transparent", mat->transparent);
			
				gui_text("Values");
				gui_separator(1);
			
				gui_drag_color("Ambient Color", mat->ambient_color);
				gui_drag_color("Diffuse Color", mat->diffuse_color);
				gui_drag_color("Specular Color", mat->specular_color);
				gui_drag_color("Emissive Color", mat->emissive_color);
				gui_drag_f32("Shininess", mat->shininess, 0.01f, 0.f, 300.f);
			}
			
			gui_end_window();
		}
	}

    SV_INTERNAL void display_gui()
    {
		if (editor.debug_draw) {
			
			if (_gui_begin()) {

				if (there_is_scene()) {

					gui_begin_top(GuiTopLocation_Left);
					{
						auto& data = editor.tool_data;

						u32 flags = (data.tool_type == EditorToolType_Gizmos) ? GuiButtonFlag_Disabled : 0;
					
						if (gui_button(NULL, 209846, GuiButtonFlag_NoBackground | flags)) {
							data.tool_type = EditorToolType_Gizmos;
						}

						flags = (data.tool_type == EditorToolType_TerrainBrush) ? GuiButtonFlag_Disabled : 0;
					
						if (gui_button(NULL, 43645, GuiButtonFlag_NoBackground | flags)) {
							data.tool_type = EditorToolType_TerrainBrush;
						}

						if (data.tool_type == EditorToolType_Gizmos) {
							
							if (gui_button(NULL, 9829344, GuiButtonFlag_NoBackground | flags)) {
								data.gizmos_data.mode = GizmosTransformMode_Position;
							}
							if (gui_button(NULL, 92745734, GuiButtonFlag_NoBackground | flags)) {
								data.gizmos_data.mode = GizmosTransformMode_Scale;
							}
						}
					}
					gui_end_top();

					gui_begin_top(GuiTopLocation_Center);

					if (dev.engine_state == EngineState_Play) {

						gui_push_image(GuiImageType_Background, editor.image.get(), engine.update_scene ? GlobalEditorData::TEXCOORD_PAUSE : GlobalEditorData::TEXCOORD_PLAY);

						if (gui_button(NULL, 754, GuiButtonFlag_NoBackground)) {
							engine.update_scene = !engine.update_scene;
						}

						gui_pop_image();

						gui_push_image(GuiImageType_Background, editor.image.get(), GlobalEditorData::TEXCOORD_STOP);
						
						if (gui_button(NULL, 324, GuiButtonFlag_NoBackground)) {
							dev.next_engine_state = EngineState_Edit;
						}

						gui_pop_image();
					}
					else {

						gui_push_image(GuiImageType_Background, editor.image.get(), GlobalEditorData::TEXCOORD_PLAY);

						if (gui_button(NULL, 324, GuiButtonFlag_NoBackground)) {
							dev.next_engine_state = EngineState_Play;
						}

						gui_pop_image();
					}

					gui_end_top();

					if (gui_begin_window("Editor View")) {

						gui_push_image(GuiImageType_Background, editor.offscreen_editor, { 0.f, 0.f, 1.f, 1.f });
						gui_image(0.f, 0, GuiImageFlag_Fullscreen);
						gui_pop_image();

						editor.editor_view_size = gui_root_size();
						editor.in_editor_view = gui_image_catch_input(0);

						if (editor.in_editor_view) {
							editor.view_position = gui_root_position();
						}

						editor.show_editor = true;

						EntityPackage* package;
						if (gui_recive_package((void**)&package, ASSET_BROWSER_ENTITY, GuiReciverTrigger_Root)) {

							load_prefab(package->filepath);
						}
					
						gui_end_window();
					}
					else editor.show_editor = false;

					if (gui_begin_window("Game View")) {

						gui_push_image(GuiImageType_Background, editor.offscreen_game, { 0.f, 0.f, 1.f, 1.f });
						gui_image(0.f, 0, GuiImageFlag_Fullscreen);
						gui_pop_image();

						editor.game_view_size = gui_root_size();
						editor.in_game_view = gui_image_catch_input(0);

						if (editor.in_game_view) {
							editor.view_position = gui_root_position();
						}

						editor.show_game = true;
					
						gui_end_window();
					}
					else editor.show_game = false;

					// Window management
					if (gui_begin_window("Window Manager", GuiWindowFlag_NoClose)) {

						const char* windows[] = {
							"Hierarchy",
							"Inspector",
							"Asset Browser",
							"Scene Settings",
							"SpriteSheet Editor",
							"Material Editor",
							"Editor View",
							"Game View",
							"Renderer Debug",
						};

						foreach(i, SV_ARRAY_SIZE(windows)) {

							const char* name = windows[i];

							if (!gui_showing_window(name)) {

								if (gui_button(name, i)) {

									gui_show_window(name);
								}
							}
						}

						gui_separator(3);

						gui_checkbox("Show Colisions", dev.draw_collisions);
						gui_checkbox("Show Cameras", dev.draw_cameras);
						gui_checkbox("Postprocessing", dev.postprocessing);

						if (gui_button("Exit Project")) {
							dev.next_engine_state = EngineState_ProjectManagement;
						}

						// TEMP
						gui_separator(2);
						{
							auto& data = editor.tool_data.terrain_brush_data;
							gui_drag_f32("Strength", data.strength, 0.1f, 0.f, f32_max);
							gui_drag_f32("Range", data.range, 0.1f, 0.f, f32_max);
							gui_drag_f32("Min Height", data.min_height, 0.1f, 0.f, f32_max);
							gui_drag_f32("Max Height", data.max_height, 0.1f, 0.f, f32_max);
						}
					
						gui_end_window();
					}
		
					display_entity_hierarchy();
					display_entity_inspector();
					display_asset_browser();
					display_scene_settings();
					display_spritesheet_editor();
					display_material_editor();
					gui_display_style_editor();

					event_dispatch("display_gui", NULL);

				}
				else {
					// TODO
				}

				_gui_end();
			}
		}

		// Change input and adjust cameras
		if (editor.debug_draw) {
			
			f32 aspect = os_window_aspect() * (SV_MAX(editor.editor_view_size.x, 0.01f) / SV_MAX(editor.editor_view_size.y, 0.01f));
			dev.camera.adjust(aspect);

			CameraComponent* camera = get_main_camera();
			if (camera) {
				aspect = os_window_aspect() * (SV_MAX(editor.game_view_size.x, 0.01f) / SV_MAX(editor.game_view_size.y, 0.01f));
				camera->adjust(aspect);
			}

			editor.absolute_mouse_position = input.mouse_position;
			editor.absolute_mouse_last_position = input.mouse_last_pos;
			editor.absolute_mouse_dragged = input.mouse_dragged;

			v2_f32 view_size = editor.editor_view_size;
			
			if (editor.in_game_view) view_size = editor.game_view_size;
			
			input.mouse_position = ((input.mouse_position + 0.5f) - (editor.view_position - view_size * 0.5f)) / view_size;
			input.mouse_position -= 0.5f;
			
			input.mouse_last_pos = ((input.mouse_last_pos + 0.5f) - (editor.view_position - view_size * 0.5f)) / view_size;
			input.mouse_last_pos -= 0.5f;

			input.mouse_dragged *= editor.editor_view_size;

			if (editor.in_editor_view || editor.camera_focus) {

				input.unused = true;
			}
		}
		else {
			
			f32 aspect = os_window_aspect();

			CameraComponent* camera = get_main_camera();
			if (camera) {
				camera->adjust(aspect);
			}
		}
    }

	SV_AUX v3_f32 compute_terrain_position(f32 h, u32 x, u32 z, u32 size_x, u32 size_z)
	{
		v3_f32 p;
		p.y = h;
	
		p.x = f32(x) / f32(size_x) - 0.5f;
		p.z = -(f32(z) / f32(size_z) - 0.5f);

		return p;
	}

	SV_AUX void update_terrain_brush()
	{
		if (input.unused) {

			bool left = input.mouse_buttons[MouseButton_Left];
			bool right = input.mouse_buttons[MouseButton_Right];

			if (left || right) {

				input.unused = false;
				
				Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);

				v3_f32 closest_pos;
				TerrainComponent* terrain = NULL;
				f32 closest_distance = f32_max;
				Entity entity = 0;

				for (CompIt it = comp_it_begin(get_component_id("Terrain"));
					 it.has_next;
					 comp_it_next(it))
				{
					TerrainComponent* t = (TerrainComponent*)it.comp;
					v3_f32 pos;
					
					if (terrain_intersect_ray(*t, it.entity, ray, pos)) {

						f32 distance = vec3_distance(pos, ray.origin);
						
						if (distance < closest_distance) {

							closest_distance = distance;
							terrain = t;
							closest_pos = pos;
							entity = it.entity;
						}
					}
				}

				if (terrain) {

					TerrainBrushData& data = editor.tool_data.terrain_brush_data;
					XMMATRIX matrix = get_entity_world_matrix(entity);

					foreach(z, terrain->resolution.y) {
						foreach(x, terrain->resolution.x) {

							f32& height = terrain->heights[x + z * terrain->resolution.x];

							v3_f32 pos = compute_terrain_position(height, x, z, terrain->resolution.x, terrain->resolution.y);
							pos = XMVector4Transform(vec3_to_dx(pos, 1.f), matrix);

							f32 distance = vec3_distance(pos, closest_pos);
							
							if (distance < data.range) {

								f32 value = ((data.range - distance) / data.range) * engine.deltatime * data.strength;

								if (left) height += value;
								if (right) height -= value;
								
								height = SV_MAX(SV_MIN(height, data.max_height), data.min_height);
							}
						}
					}

					terrain->dirty = true;
				}
			}
		}
	}

		

    SV_INTERNAL void do_picking_stuff()
    {
		// Update tool
		{
			EditorToolData& data = editor.tool_data;
			
			switch (data.tool_type) {

			case EditorToolType_TerrainBrush:
				update_terrain_brush();
				break;

			case EditorToolType_Gizmos:
				update_gizmos();
				break;
				
			}
		}
	
		// Entity selection
		if (input.unused && input.mouse_buttons[MouseButton_Left] == InputState_Released)
			select_entity();
    }
    
    SV_INTERNAL void update_editor_stuff()
    {
		for (u32 i = 0u; i < editor.selected_entities.size();) {

			if (!entity_exists(editor.selected_entities[i]))
				editor.selected_entities.erase(i);
			else ++i;
		}

		if (editor.selected_entities.empty())
			editor.entity_viewer = false;
	
		display_gui();

		if (there_is_scene()) {

			control_camera();
		}

		if (there_is_scene())
			do_picking_stuff();
    }

    void update_project_state()
    {
		if (_gui_begin()) {

			// TEST
			{
				if (gui_begin_window("Test")) {

					TextureAsset tex;

					if (load_asset_from_file(tex, "$system/images/skymap.jpeg")) {
						gui_push_image(GuiImageType_Background, tex.get(), { 0.f, 0.f, 1.f, 1.f });
						gui_image(0.f, 0u, GuiImageFlag_Fullscreen);
					}
		    
					gui_end_window();
				}
			}

			if (gui_button("New project", 0u)) {
				
				char path[FILEPATH_SIZE + 1u] = "";
		    
				if (file_dialog_save(path, 0u, nullptr, "")) {

					char* extension = filepath_extension(path);
					if (extension != nullptr) {
						*extension = '\0';
					}
					string_append(path, ".silver", FILEPATH_SIZE + 1u);

					const char* content = "test";
					bool res = file_write_text(path, content, strlen(content));

					if (res) {
			    
						char* name = filepath_name(path);
						*name = '\0';

						char folderpath[FILEPATH_SIZE + 1u];
			    
						sprintf(folderpath, "%s%s", path, "assets");
						res = folder_create(folderpath);
			    
						if (res) {
							sprintf(folderpath, "%s%s", path, "src");
							res = folder_create(folderpath);
						}

						if (res) {

							sprintf(folderpath, "%s%s", path, "src/build_unit.cpp");
							res = file_copy("$system/default_code.cpp", folderpath);
						}
					}
		    
			
					if (res)
						SV_LOG_INFO("Project in '%s' created", path);
		    
					else {
						SV_LOG_ERROR("Can't create the project in '%s'", path);
					}
				}
			}
			if (gui_button("Open project", 1u)) {

				char path[FILEPATH_SIZE + 1u] = "";

				const char* filter[] = {
					"Silver Engine (.silver)", "*.silver",
					"All", "*",
					""
				};

				if (file_dialog_open(path, 2u, filter, "")) {
			
					*filepath_name(path) = '\0';
					_engine_initialize_project(path);
					dev.next_engine_state = EngineState_Edit;
				}
			}
	    
			_gui_end();
		}    
    }
    
    void _editor_update()
    {
		bool exit = false;
	
		// CHANGE EDITOR MODE
		if (dev.next_engine_state != EngineState_None) {

			switch (dev.next_engine_state) {

			case EngineState_ProjectManagement:
			{
				_engine_close_project();
				editor.selected_entities.reset();
				exit = true;
				engine.update_scene = false;
				editor.debug_draw = true;
				editor.show_editor = false;
				_gui_load("PROJECT");
			}
			break;

			case EngineState_Edit:
			{
				SV_LOG_INFO("Starting edit state");
				// TODO: Handle error
				if (dev.engine_state != EngineState_ProjectManagement)
					_engine_reset_game(get_scene_name());
				
				//dev.draw_debug_camera = true;
				editor.debug_draw = true;
				engine.update_scene = false;
				_gui_load(engine.project_path);
			} break;

			case EngineState_Play:
			{
				SV_LOG_INFO("Starting play state");

				engine.update_scene = true;
			
				if (dev.engine_state == EngineState_Edit) {

					// TODO: handle error
					save_scene();
					_engine_reset_game(get_scene_name());

					//dev.draw_debug_camera = false;
					editor.selected_entities.reset();
				}
			} break;

			case EngineState_Pause:
			{
				SV_LOG_INFO("Game paused");
				engine.update_scene = false;
			} break;
			
			}

			dev.engine_state = dev.next_engine_state;
			dev.next_engine_state = EngineState_None;

			if (exit)
				return;
		}

		update_key_shortcuts();
	
		switch (dev.engine_state) {

		case EngineState_Edit:
		case EngineState_Play:
			update_editor_stuff();
			break;

		case EngineState_ProjectManagement:
			update_project_state();
			break;
	    
		}
    }

    SV_INTERNAL void draw_edit_state(CommandList cmd)
    {
		//if (!dev.debug_draw) return;
	
		imrend_begin_batch(cmd);

		imrend_camera(ImRendCamera_Editor, cmd);

		u32 light_id = get_component_id("Light");
		u32 mesh_id = get_component_id("Mesh");
		u32 sprite_id = get_component_id("Sprite");
		u32 body_id = get_component_id("Body");
		u32 box_id = get_component_id("Box Collider");
		u32 sphere_id = get_component_id("Sphere Collider");
		u32 camera_id = get_component_id("Camera");

		// Draw selected entity
		for (Entity entity : editor.selected_entities) {

			MeshComponent* mesh_comp = (MeshComponent*)get_entity_component(entity, mesh_id);
			SpriteComponent* sprite_comp = (SpriteComponent*)get_entity_component(entity, sprite_id);

			if (mesh_comp && mesh_comp->mesh.get()) {

				u8 alpha = 5u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 30.f * 0.5f);
				XMMATRIX wm = get_entity_world_matrix(entity);

				imrend_push_matrix(wm, cmd);
				imrend_draw_mesh_wireframe(mesh_comp->mesh.get(), Color::Red(alpha), cmd);
				imrend_pop_matrix(cmd);
			}
			if (sprite_comp) {

				XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

				XMMATRIX tm = get_entity_world_matrix(entity);

				p0 = XMVector3Transform(p0, tm);
				p1 = XMVector3Transform(p1, tm);
				p2 = XMVector3Transform(p2, tm);
				p3 = XMVector3Transform(p3, tm);

				u8 alpha = 50u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 200.f * 0.5f);
				Color selection_color = Color::Red(alpha);

				imrend_draw_line(v3_f32(p0), v3_f32(p1), selection_color, cmd);
				imrend_draw_line(v3_f32(p1), v3_f32(p3), selection_color, cmd);
				imrend_draw_line(v3_f32(p3), v3_f32(p2), selection_color, cmd);
				imrend_draw_line(v3_f32(p2), v3_f32(p0), selection_color, cmd);
			}
		}

		// Draw 2D grid
		if (dev.camera.projection_type == ProjectionType_Orthographic) {

			f32 width = dev.camera.width;
			f32 height = dev.camera.height;
			f32 mag = dev.camera.getProjectionLength();

			u32 count = 0u;
			for (f32 i = 0.01f; count < 3u; i *= 10.f) {

				if (mag / i <= 50.f) {

					Color color;

					switch (count++)
					{
					case 0:
						color = Color::Gray(50);
						break;

					case 1:
						color = Color::Gray(100);
						break;

					case 2:
						color = Color::Gray(150);
						break;

					case 3:
						color = Color::Gray(200);
						break;
					}

					color.a = 10u;

					imrend_draw_orthographic_grip(vec3_to_vec2(dev.camera.position), {}, { width, height }, i, color, cmd);
				}
			}
		}

		// Draw light probes
		{
			XMMATRIX tm;
			
			for (CompIt it = comp_it_begin(light_id);
				 it.has_next;
				 comp_it_next(it))
			{
				Entity entity = it.entity;
				LightComponent& light = *(LightComponent*)it.comp;
				
				v3_f32 pos = get_entity_world_position(entity);
				
				f32 min_scale = relative_scalar(0.02f, pos);
				f32 scale = SV_MAX(min_scale, 1.f);
					
				tm = XMMatrixScaling(scale, scale, 1.f) * XMMatrixRotationQuaternion(vec4_to_dx(dev.camera.rotation)) * XMMatrixTranslation(pos.x, pos.y, pos.z);

				imrend_push_matrix(tm, cmd);

				imrend_draw_sprite({}, { 1.f, 1.f }, Color::White(), editor.image.get(), GPUImageLayout_ShaderResource, editor.TEXCOORD_LIGHT_PROBE, cmd);

				imrend_pop_matrix(cmd);

				if (is_entity_selected(entity) && light.light_type == LightType_Direction) {

					// Draw light direction

					v3_f32 dir = v3_f32::forward();

					XMVECTOR quat = vec4_to_dx(get_entity_world_rotation(entity));

					tm = XMMatrixRotationQuaternion(quat);

					dir = XMVector3Transform(vec3_to_dx(dir), tm);

					dir = vec3_normalize(dir) * scale * 1.5f;
					imrend_draw_line(pos, pos + dir, light.color, cmd);
				}	
			}
		}

		// Draw collisions
		if (dev.draw_collisions) {

			XMMATRIX tm;

			for (CompIt it = comp_it_begin(box_id);
				 it.has_next;
				 comp_it_next(it))
			{
				BoxCollider& box = *(BoxCollider*)it.comp;

				if (!has_entity_component(it.entity, body_id))
					continue;
					
				tm = XMMatrixScalingFromVector(vec3_to_dx(box.size)) * get_entity_world_matrix(it.entity);

				imrend_push_matrix(tm, cmd);
				imrend_draw_cube_wireframe(Color::Green(), cmd);
				imrend_pop_matrix(cmd);
			}

			for (CompIt it = comp_it_begin(sphere_id);
				 it.has_next;
				 comp_it_next(it))
			{
				SphereCollider& sphere = *(SphereCollider*)it.comp;

				if (!has_entity_component(it.entity, body_id))
					continue;

				Transform trans = get_entity_world_transform(it.entity);

				f32 radius = sphere.radius * SV_MAX(SV_MAX(trans.scale.x, trans.scale.y), trans.scale.z);
				
				tm = XMMatrixScaling(radius, radius, radius) *
					XMMatrixRotationQuaternion(vec4_to_dx(trans.rotation)) *
					XMMatrixTranslationFromVector(vec3_to_dx(trans.position));

				imrend_push_matrix(tm, cmd);
				imrend_draw_sphere_wireframe(10u, 10u, Color::Green(), cmd);
				imrend_pop_matrix(cmd);
			}
		}

		if (dev.draw_cameras) {

			XMMATRIX tm;

			for (CompIt it = comp_it_begin(camera_id);
				 it.has_next;
				 comp_it_next(it))
			{
				CameraComponent& camera = *(CameraComponent*)it.comp;

				imrend_push_matrix(camera.inverse_view_matrix, cmd);

				v3_f32 b0 = { -camera.width * 0.5f, camera.height * 0.5f, camera.near };
				v3_f32 b1 = { camera.width * 0.5f, camera.height * 0.5f, camera.near };
				v3_f32 b2 = { -camera.width * 0.5f, -camera.height * 0.5f, camera.near };
				v3_f32 b3 = { camera.width * 0.5f, -camera.height * 0.5f, camera.near };

				v3_f32 e0;
				v3_f32 e1;
				v3_f32 e2;
				v3_f32 e3;

				if (camera.projection_type == ProjectionType_Orthographic) {

					e0 = { b0.x, b0.y, camera.far };
					e1 = { b1.x, b1.y, camera.far };
					e2 = { b2.x, b2.y, camera.far };
					e3 = { b3.x, b3.y, camera.far };
				}
				if (camera.projection_type == ProjectionType_Perspective) {

					f32 distance = vec3_length(b0);

					// TODO: Thats wrong
					e0 = vec3_normalize(b0) * (distance + camera.far);
					e1 = vec3_normalize(b1) * (distance + camera.far);
					e2 = vec3_normalize(b2) * (distance + camera.far);
					e3 = vec3_normalize(b3) * (distance + camera.far);
				}

				imrend_draw_line(b0, e0, Color::Red(), cmd);
				imrend_draw_line(b1, e1, Color::Red(), cmd);
				imrend_draw_line(b2, e2, Color::Red(), cmd);
				imrend_draw_line(b3, e3, Color::Red(), cmd);

				imrend_draw_line(b0, b1, Color::Red(), cmd);
				imrend_draw_line(b1, b3, Color::Red(), cmd);
				imrend_draw_line(b3, b2, Color::Red(), cmd);
				imrend_draw_line(b2, b0, Color::Red(), cmd);

				imrend_draw_line(e0, e1, Color::Red(), cmd);
				imrend_draw_line(e1, e3, Color::Red(), cmd);
				imrend_draw_line(e3, e2, Color::Red(), cmd);
				imrend_draw_line(e2, e0, Color::Red(), cmd);

				imrend_pop_matrix(cmd);

				v3_f32 pos = get_entity_world_position(it.entity);
				
				f32 min_scale = relative_scalar(0.03f, pos);
				f32 scale = SV_MAX(min_scale, 1.f);
					
				tm = XMMatrixScaling(scale, scale, 1.f) * XMMatrixRotationQuaternion(vec4_to_dx(dev.camera.rotation)) * XMMatrixTranslation(pos.x, pos.y, pos.z);

				imrend_push_matrix(tm, cmd);

				imrend_draw_sprite({}, { 1.f, 1.f }, Color::White(), editor.image.get(), GPUImageLayout_ShaderResource, editor.TEXCOORD_CAMERA_PROBE, cmd);

				imrend_pop_matrix(cmd);
			}
		}

		// Draw gizmos
		switch (editor.tool_data.tool_type) {

		case EditorToolType_Gizmos:
			draw_gizmos(renderer->gfx.offscreen, cmd);
			break;

		}

		imrend_flush(cmd);
    }

    void _editor_draw()
    {
		CommandList cmd = graphics_commandlist_get();

		if (editor.debug_draw) {

			if (editor.show_editor) {

				_draw_scene(dev.camera, dev.camera.position, dev.camera.rotation);
				draw_edit_state(cmd);

				GPUImage* off = renderer_offscreen();
				const GPUImageInfo& info = graphics_image_info(off);
			
				GPUImageBlit blit;
				blit.src_region.offset0 = { 0, (i32)info.height, 0 };
				blit.src_region.offset1 = { (i32)info.width, 0, 1 };
				blit.dst_region.offset0 = { 0, 0, 0 };
				blit.dst_region.offset1 = { (i32)info.width, (i32)info.height, 1 };
			
				graphics_image_blit(off, editor.offscreen_editor, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource, 1u, &blit, SamplerFilter_Nearest, cmd);
			}

			if (editor.show_game) {

				_draw_scene();
			
				GPUImage* off = renderer_offscreen();
				const GPUImageInfo& info = graphics_image_info(off);
			
				GPUImageBlit blit;
				blit.src_region.offset0 = { 0, (i32)info.height, 0 };
				blit.src_region.offset1 = { (i32)info.width, 0, 1 };
				blit.dst_region.offset0 = { 0, 0, 0 };
				blit.dst_region.offset1 = { (i32)info.width, (i32)info.height, 1 };
			
				graphics_image_blit(off, editor.offscreen_game, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource, 1u, &blit, SamplerFilter_Nearest, cmd);
			}

			_gui_draw(cmd);
		}
		else _draw_scene();
    }

	List<Entity>& editor_selected_entities()
	{
		return editor.selected_entities;
	}

}

#endif
