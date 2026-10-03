// Validate the physics configurations and collision meshes installed to svencoop_downloads.
#include <cassert>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "BulletPhysicManager.h"
#include "PhysicUTIL.h"
#include "test_support.h"

static std::filesystem::path s_GameDirectory;

static bool EndsWith(const std::string& text, const std::string& suffix)
{
	return text.size() >= suffix.size() && 0 == text.compare(text.size() - suffix.size(), suffix.size(), suffix);
}

// "@barnacle.Body" style names bind to another object's rigid body at runtime.
static bool IsExternalRigidBody(const std::string& name)
{
	return !name.empty() && '@' == name[0];
}

// UTIL_GetCrc32ForBoneChunk produces eight lowercase hex digits and the check compares strings.
static bool IsCrc32Text(const std::string& text)
{
	if (8 != text.size())
		return false;
	for (char c : text)
		if (!std::isdigit((unsigned char)c) && (c < 'a' || c > 'f'))
			return false;
	return true;
}

static void CheckTriangleMeshResource(const std::string& resourcePath)
{
	assert(!resourcePath.empty());
	assert(std::filesystem::is_regular_file(s_GameDirectory / resourcePath));

	auto pIndexArray = ClientPhysicManager()->LoadIndexArrayFromResource(resourcePath);
	assert(pIndexArray);
	assert(PhysicIndexArrayFlag_FromOBJ == (pIndexArray->flags & (PhysicIndexArrayFlag_FromOBJ | PhysicIndexArrayFlag_LoadFailed)));
	const auto vertexCount = pIndexArray->pVertexArray->vVertexBuffer.size();
	assert(vertexCount > 0);
	assert(!pIndexArray->vIndexBuffer.empty());
	assert(0 == pIndexArray->vIndexBuffer.size() % 3);
	for (int index : pIndexArray->vIndexBuffer)
		assert(index >= 0 && (size_t)index < vertexCount);
	// Every object using the mesh shares one cached copy.
	assert(pIndexArray == ClientPhysicManager()->LoadIndexArrayFromResource(resourcePath));
}

static void CheckCollisionShapeResources(const CClientCollisionShapeConfig* pShape)
{
	assert(PhysicShape_None != pShape->type);
	if (PhysicShape_TriangleMesh == pShape->type)
		CheckTriangleMeshResource(pShape->resourcePath);
	for (const auto& pChild : pShape->compoundShapes)
		CheckCollisionShapeResources(pChild.get());
}

static void DeleteCollisionShape(btCollisionShape* pShape)
{
	if (pShape->isCompound())
	{
		auto pCompound = static_cast<btCompoundShape*>(pShape);
		for (int i = pCompound->getNumChildShapes() - 1; i >= 0; --i)
			DeleteCollisionShape(pCompound->getChildShape(i));
	}
	OnBeforeDeleteBulletCollisionShape(pShape);
	delete pShape;
}

static void CheckBulletCollisionShape(const CClientRigidBodyConfig* pRigidBody)
{
	auto pShape = BulletCreateCollisionShape(pRigidBody);
	assert(pShape);

	// A flat or empty volume lets the body fall through the world.
	btTransform identity;
	identity.setIdentity();
	btVector3 aabbMin;
	btVector3 aabbMax;
	pShape->getAabb(identity, aabbMin, aabbMax);
	for (int axis = 0; axis < 3; ++axis)
		assert(aabbMax[axis] > aabbMin[axis]);

	DeleteCollisionShape(pShape);
}

static void CheckConfigConsistency(const CClientPhysicObjectConfig* pConfig)
{
	// The editor saves components keyed by name, so duplicates would be merged.
	std::set<std::string> rigidBodies;
	std::set<std::string> constraints;
	std::set<std::string> physicBehaviors;

	for (const auto& pRigidBody : pConfig->RigidBodyConfigs)
	{
		assert(!pRigidBody->name.empty());
		assert(rigidBodies.insert(pRigidBody->name).second);
		assert(pRigidBody->collisionShape);
		CheckCollisionShapeResources(pRigidBody->collisionShape.get());
		CheckBulletCollisionShape(pRigidBody.get());
	}

	for (const auto& pConstraint : pConfig->ConstraintConfigs)
	{
		assert(!pConstraint->name.empty());
		assert(constraints.insert(pConstraint->name).second);
		assert(PhysicConstraint_None != pConstraint->type);
		for (const auto& name : { pConstraint->rigidbodyA, pConstraint->rigidbodyB })
			assert(IsExternalRigidBody(name) || rigidBodies.contains(name));
	}

	for (const auto& pPhysicBehavior : pConfig->PhysicBehaviorConfigs)
	{
		assert(!pPhysicBehavior->name.empty());
		assert(physicBehaviors.insert(pPhysicBehavior->name).second);
		assert(PhysicBehavior_None != pPhysicBehavior->type);
		for (const auto& name : { pPhysicBehavior->rigidbodyA, pPhysicBehavior->rigidbodyB })
			assert(name.empty() || IsExternalRigidBody(name) || rigidBodies.contains(name));
		assert(pPhysicBehavior->constraint.empty() || constraints.contains(pPhysicBehavior->constraint));
	}

	if (pConfig->verifyBoneChunk)
		assert(IsCrc32Text(pConfig->crc32BoneChunk));
	if (pConfig->verifyModelFile)
		assert(IsCrc32Text(pConfig->crc32ModelFile));
}

static void CheckComponentsSurviveEditorSave(const CClientPhysicObjectConfig* pConfig)
{
	auto pSaved = TestLoadPhysicObjectConfigFromText(TestSavePhysicObjectConfigToText(pConfig), TestGetModelWithoutStudioData());
	assert(pSaved);
	assert(pConfig->type == pSaved->type);

	assert(pConfig->RigidBodyConfigs.size() == pSaved->RigidBodyConfigs.size());
	for (size_t i = 0; i < pConfig->RigidBodyConfigs.size(); ++i)
	{
		assert(pConfig->RigidBodyConfigs[i]->name == pSaved->RigidBodyConfigs[i]->name);
		assert(pConfig->RigidBodyConfigs[i]->collisionShape->type == pSaved->RigidBodyConfigs[i]->collisionShape->type);
	}

	assert(pConfig->ConstraintConfigs.size() == pSaved->ConstraintConfigs.size());
	for (size_t i = 0; i < pConfig->ConstraintConfigs.size(); ++i)
	{
		const auto& pExpected = pConfig->ConstraintConfigs[i];
		const auto& pActual = pSaved->ConstraintConfigs[i];
		assert(pExpected->name == pActual->name);
		assert(pExpected->type == pActual->type);
		for (int index = 0; index < PhysicConstraintFactorIdx_Maximum; ++index)
			assert(IsSameFactor(pExpected->factors[index], pActual->factors[index]));
	}

	assert(pConfig->PhysicBehaviorConfigs.size() == pSaved->PhysicBehaviorConfigs.size());
	for (size_t i = 0; i < pConfig->PhysicBehaviorConfigs.size(); ++i)
	{
		const auto& pExpected = pConfig->PhysicBehaviorConfigs[i];
		const auto& pActual = pSaved->PhysicBehaviorConfigs[i];
		assert(pExpected->name == pActual->name);
		assert(pExpected->type == pActual->type);
		assert(IsNearVector(pExpected->origin, pActual->origin));
		assert(IsNearVector(pExpected->angles, pActual->angles));
		for (int index = 0; index < PhysicBehaviorFactorIdx_Maximum; ++index)
			assert(IsSameFactor(pExpected->factors[index], pActual->factors[index]));
	}

	if (PhysicObjectType_RagdollObject == pConfig->type)
	{
		auto pRagdoll = static_cast<const CClientRagdollObjectConfig*>(pConfig);
		auto pSavedRagdoll = static_cast<const CClientRagdollObjectConfig*>(pSaved.get());
		assert(pRagdoll->AnimControlConfigs.size() == pSavedRagdoll->AnimControlConfigs.size());
	}
}

int main(int argc, char** argv)
{
	assert(2 == argc);
	s_GameDirectory = argv[1];
	TestInitPluginRuntime(s_GameDirectory);

	int physicsConfigCount = 0;
	int legacyConfigCount = 0;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(s_GameDirectory))
	{
		if (!entry.is_regular_file())
			continue;

		const auto fileName = entry.path().filename().string();
		const auto text = TestReadTextFile(entry.path());
		std::shared_ptr<CClientPhysicObjectConfig> pConfig;

		if (EndsWith(fileName, "_physics.txt"))
		{
			// Models are not shipped, so integrity checksums are validated for format only.
			pConfig = TestLoadPhysicObjectConfigFromText(text, TestGetModelWithoutStudioData());
			++physicsConfigCount;
		}
		else if (EndsWith(fileName, "_ragdoll.txt"))
		{
			pConfig = LoadPhysicObjectConfigFromLegacyFileBuffer(text.c_str());
			++legacyConfigCount;
		}
		else
		{
			continue;
		}

		printf("Checking %s\n", entry.path().lexically_relative(s_GameDirectory).generic_string().c_str());
		assert(pConfig);
		assert(PhysicObjectFlag_FromConfig & pConfig->flags);
		CheckConfigConsistency(pConfig.get());
		CheckComponentsSurviveEditorSave(pConfig.get());
	}

	// Guard against a wrong directory passing vacuously.
	assert(physicsConfigCount > 0);
	assert(legacyConfigCount > 0);

	TestShutdownPluginRuntime();
	printf("Shipped physics asset tests passed: %d *_physics.txt, %d *_ragdoll.txt.\n", physicsConfigCount, legacyConfigCount);
	return 0;
}
