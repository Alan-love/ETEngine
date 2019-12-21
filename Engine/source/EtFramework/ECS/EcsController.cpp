#include "stdafx.h"
#include "EcsController.h"

#include "ComponentSignature.h"
#include "Archetype.h"


namespace framework {


//================
// ECS Controller
//================


// construct destruct
//////////////////////

//--------------------------
// EcsController::d-tor
//
EcsController::~EcsController()
{
	for (ArchetypeContainer& container : m_HierachyLevels)
	{
		for (std::pair<T_Hash const, Archetype*>& arch : container.archetypes)
		{
			delete arch.second;
		}
	}
}

// entity managment
/////////////////////

//--------------------------
// EcsController::AddEntity
//
// Add an entity without components to the system
//
T_EntityId EcsController::AddEntity()
{
	return AddEntityChild(INVALID_ENTITY_ID);
}

//-------------------------------
// EcsController::AddEntityChild
//
// Add an entity without components to the system, one level below the parent entity
//
T_EntityId EcsController::AddEntityChild(T_EntityId const parent)
{
	return AddEntityBatched(parent, {});
}

//---------------------------------
// EcsController::AddEntityBatched
//
// Add an entity with a set of components
//
T_EntityId EcsController::AddEntityBatched(std::vector<RawComponentPtr> const& components)
{
	return AddEntityBatched(INVALID_ENTITY_ID, components);
}

//----------------------------------
// EcsController::AddEntityBatched
//
// Add an entity with a set of components
//  - the entity is linked to the parent and placed in the layer below
//
T_EntityId EcsController::AddEntityBatched(T_EntityId const parent, std::vector<RawComponentPtr> const& components)
{
	// create the entity data
	auto ent = m_Entities.insert(EntityData());

	// deal with hierachy issues
	if (parent != INVALID_ENTITY_ID)
	{
		ent.first->parent = parent;

		EntityData& parentEnt = m_Entities[parent];
		parentEnt.children.push_back(ent.second);

		ent.first->layer = parentEnt.layer + 1u;
	}

	// find the archetype for the new component list
	ent.first->archetype = FindOrCreateArchetype(ComponentSignature(components), ent.first->layer);
	ent.first->index = ent.first->archetype->AddEntity(ent.second, components);

	// return the ID
	return ent.second;
}

//-------------------------------
// EcsController::ReparentEntity
//
// Change what parent an entity is linked to, also moving all of its children
//  - if the parent is not set to INVALID_ENTITY_ID, the entity is linked to the parent and placed in the layer below
//
void EcsController::ReparentEntity(T_EntityId const entity, T_EntityId const newParent)
{
	// get referred entity
	EntityData& ent = m_Entities[entity];

	// remove from current parent
	RemoveEntityFromParent(entity, ent.parent);

	std::vector<RawComponentPtr> components;
	T_CompTypeList compTypes = GetComponentsAndTypes(ent, components);

	// add to new parent, and get new layer
	ent.parent = newParent;
	if (ent.parent != INVALID_ENTITY_ID)
	{
		EntityData& parentEnt = m_Entities[ent.parent];
		parentEnt.children.push_back(entity);

		ent.layer = parentEnt.layer + 1u;
	}
	else
	{
		ent.layer = 0u;
	}

	// #todo: handle cases in which the hierachy level didn't change
	// the archetype will have the same signature but reside on a new hierachy layer
	MoveArchetype(entity, ent, compTypes, components);

	// recursively reparent children to match hierachy layers
	for (T_EntityId const childId : ent.children)
	{
		ReparentEntity(childId, entity);
	}
}

//-----------------------------
// EcsController::RemoveEntity
//
// Remove an entity and all its children
//
void EcsController::RemoveEntity(T_EntityId const entity)
{
	// get referred entity
	EntityData& ent = m_Entities[entity];

	RemoveEntityFromParent(entity, ent.parent);

	RemoveEntityFromArchetype(ent);

	// recursively remove children
	for (T_EntityId const childId : ent.children)
	{
		RemoveEntity(childId);
	}

	// remove from entity map
	m_Entities.erase(entity);
}

//----------------------------------
// EcsController::RemoveAllEntities
//
// While the entities and their components are removed, the now empty archetypes persist
//
void EcsController::RemoveAllEntities()
{
	m_Entities.clear();

	for (ArchetypeContainer& level : m_HierachyLevels)
	{
		for (std::pair<T_Hash const, Archetype*>& arch : level.archetypes)
		{
			arch.second->Clear();
		}
	}
}


// component managment
///////////////////////

//-----------------------------
// EcsController::AddComponent
//
// Add a list of components to an entity
//
void EcsController::AddComponents(T_EntityId const entity, std::vector<RawComponentPtr>& components)
{
	// get referred entity
	EntityData& ent = m_Entities[entity];

	std::vector<RawComponentPtr> currentComponents;
	T_CompTypeList compTypes = GetComponentsAndTypes(ent, currentComponents);

	// add the new components
	for (RawComponentPtr const& comp : components)
	{
		compTypes.emplace_back(comp.typeIdx);
		currentComponents.emplace_back(comp);
	}

	MoveArchetype(entity, ent, compTypes, currentComponents);

	// reassign the component pointers
	for (RawComponentPtr& comp : components)
	{
		comp.data = ent.archetype->GetPool(comp.typeIdx).At(ent.index);
	}
}

//---------------------------------
// EcsController::RemoveComponents
//
// Remove a list of components from an entity
//
void EcsController::RemoveComponents(T_EntityId const entity, T_CompTypeList const& componentTypes)
{
	// get referred entity
	EntityData& ent = m_Entities[entity];

	std::vector<RawComponentPtr> currentComponents;
	T_CompTypeList compTypes = GetComponentsAndTypes(ent, currentComponents);

	// remove the components
	for (T_CompTypeIdx const comp : componentTypes)
	{
		if (compTypes.size() == 1u)
		{
			ET_ASSERT(compTypes[0u] == comp);
			compTypes.clear();

			ET_ASSERT(currentComponents.size() == 1u);
			ET_ASSERT(currentComponents[0u].typeIdx == comp);
			currentComponents.clear();
		}
		else
		{
			auto const foundType = std::find(compTypes.begin(), compTypes.end(), comp);
			ET_ASSERT(foundType != compTypes.cend());

			size_t const idx = foundType - compTypes.cbegin();
			ET_ASSERT(compTypes[idx] == currentComponents[idx].typeIdx);

			compTypes[idx] = compTypes[compTypes.size() - 1];
			compTypes.pop_back();

			currentComponents[idx] = currentComponents[currentComponents.size() - 1];
			currentComponents.pop_back();
		}
	}

	MoveArchetype(entity, ent, compTypes, currentComponents);
}


// system managment
////////////////////

//-----------------------
// EcsController::Update
//
// Update all systems according to their implicit schedule
//
void EcsController::Update()
{
	ET_ASSERT(false, "not implemented!");
}


// accessors
/////////////

//--------------------------
// EcsController::HasParent
//
bool EcsController::HasParent(T_EntityId const entity) const
{
	return (GetParent(entity) != INVALID_ENTITY_ID);
}

//--------------------------
// EcsController::GetParent
//
T_EntityId EcsController::GetParent(T_EntityId const entity) const
{
	return m_Entities[entity].parent;
}

//----------------------------
// EcsController::GetChildren
//
std::vector<T_EntityId> const& EcsController::GetChildren(T_EntityId const entity) const
{
	return m_Entities[entity].children;
}

//----------------------------------
// EcsController::GetComponentTypes
//
std::vector<T_CompTypeIdx> const& EcsController::GetComponentTypes(T_EntityId const entity) const
{
	return m_Entities[entity].archetype->GetSignature().GetTypes();
}

//-----------------------------
// EcsController::HasComponent
//
bool EcsController::HasComponent(T_EntityId const entity, T_CompTypeIdx const compType) const
{
	return m_Entities[entity].archetype->HasComponent(compType);
}

//---------------------------------
// EcsController::GetComponentData
//
void* EcsController::GetComponentData(T_EntityId const entity, T_CompTypeIdx const compType) 
{
	EntityData& ent = m_Entities[entity];

	return ent.archetype->GetPool(compType).At(ent.index);
}

//---------------------------------
// EcsController::GetComponentData
//
void const* EcsController::GetComponentData(T_EntityId const entity, T_CompTypeIdx const compType) const
{
	EntityData const& ent = m_Entities[entity];

	return ent.archetype->GetPool(compType).At(ent.index);
}


// utility
///////////

//--------------------------------------
// EcsController::FindOrCreateArchetype
//
// Get the associated archetype or create a new one
//
Archetype* EcsController::FindOrCreateArchetype(ComponentSignature const& sig, uint8 const layer)
{
	T_Hash const sigId = sig.GenId();

	// ensure we have an archetype container for the hierachy layer we are on
	while (m_HierachyLevels.size() <= static_cast<size_t>(layer))
	{
		m_HierachyLevels.emplace_back(ArchetypeContainer());
	}

	// find or create
	ArchetypeContainer& cont = m_HierachyLevels[layer];
	auto foundA = cont.archetypes.find(sigId);
	if (foundA == cont.archetypes.cend())
	{
		auto res = cont.archetypes.emplace(sigId, new Archetype(sig));
		ET_ASSERT(res.second == true);
		foundA = res.first;
	}

	return foundA->second;
}

//------------------------------
// EcsController::MoveArchetype
//
// Move an entities component data to the correct archetype
//
void EcsController::MoveArchetype(T_EntityId const entId, EntityData& ent, T_CompTypeList const& compTypes, std::vector<RawComponentPtr> const& components)
{
	// find the archetype for the new component list
	Archetype* const nextA = FindOrCreateArchetype(ComponentSignature(compTypes), ent.layer);

	// add our entity to the next archetype
	size_t const nextIdx = nextA->AddEntity(entId, components);

	RemoveEntityFromArchetype(ent);

	// reassign the current entity
	ent.archetype = nextA;
	ent.index = nextIdx;
}

//------------------------------------------
// EcsController::RemoveEntityFromArchetype
//
// Remove the entity data from its current archetype, and ensure swapping is correctly handled
//
void EcsController::RemoveEntityFromArchetype(EntityData& ent)
{
	T_EntityId const changedEnt = ent.archetype->RemoveEntity(ent.index);
	if (changedEnt != INVALID_ENTITY_ID) // since the archetype will swap, we need to reassign the index of the entity that was swapped in
	{
		m_Entities[changedEnt].index = ent.index;
	}
}

//--------------------------------------
// EcsController::GetComponentsAndTypes
//
T_CompTypeList EcsController::GetComponentsAndTypes(EntityData& ent, std::vector<RawComponentPtr>& components)
{
	T_CompTypeList compTypes = ent.archetype->GetSignature().GetTypes();
	for (T_CompTypeIdx const type : compTypes)
	{
		components.emplace_back(type, ent.archetype->GetPool(type).At(ent.index));
	}

	return compTypes;
}

//------------------------------
// EcsController::MoveArchetype
//
// Unmap the entity from the parent - assumes parent is actually a parent of the entity
//
void EcsController::RemoveEntityFromParent(T_EntityId const entity, T_EntityId const parent)
{
	if (parent != INVALID_ENTITY_ID)
	{
		EntityData& parentEnt = m_Entities[parent];
		if (parentEnt.children.size() == 1u)
		{
			ET_ASSERT(parentEnt.children[0u] == entity);
			parentEnt.children.clear();
		}
		else
		{
			auto const foundChild = std::find(parentEnt.children.begin(), parentEnt.children.end(), entity);
			ET_ASSERT(foundChild != parentEnt.children.cend());

			size_t const idx = foundChild - parentEnt.children.cbegin();
			parentEnt.children[idx] = parentEnt.children[parentEnt.children.size() - 1];
			parentEnt.children.pop_back();
		}
	}
}


} // namespace framework