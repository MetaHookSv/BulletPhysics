// Exercise the physics configuration formats with the plugin's real loaders and serializers.
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string>

#include "exportfuncs.h"
#include "privatehook.h"
#include "PhysicUTIL.h"
#include "test_support.h"

static void SetVector(vec3_t vec, float x, float y, float z)
{
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
}

static bool Contains(const std::string& text, const char* fragment)
{
	return std::string::npos != text.find(fragment);
}

// Factors that the *_physics.txt format stores for each constraint type.
static std::set<int> GetPersistedConstraintFactors(int type)
{
	const std::set<int> dof6Factors = {
		PhysicConstraintFactorIdx_Dof6LowerLinearLimitX, PhysicConstraintFactorIdx_Dof6LowerLinearLimitY, PhysicConstraintFactorIdx_Dof6LowerLinearLimitZ,
		PhysicConstraintFactorIdx_Dof6UpperLinearLimitX, PhysicConstraintFactorIdx_Dof6UpperLinearLimitY, PhysicConstraintFactorIdx_Dof6UpperLinearLimitZ,
		PhysicConstraintFactorIdx_Dof6LowerAngularLimitX, PhysicConstraintFactorIdx_Dof6LowerAngularLimitY, PhysicConstraintFactorIdx_Dof6LowerAngularLimitZ,
		PhysicConstraintFactorIdx_Dof6UpperAngularLimitX, PhysicConstraintFactorIdx_Dof6UpperAngularLimitY, PhysicConstraintFactorIdx_Dof6UpperAngularLimitZ,
		PhysicConstraintFactorIdx_LinearCFM, PhysicConstraintFactorIdx_LinearStopERP, PhysicConstraintFactorIdx_LinearStopCFM,
		PhysicConstraintFactorIdx_AngularCFM, PhysicConstraintFactorIdx_AngularStopERP, PhysicConstraintFactorIdx_AngularStopCFM,
		PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset,
	};

	switch (type)
	{
	case PhysicConstraint_ConeTwist:
		return {
			PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit1, PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit2, PhysicConstraintFactorIdx_ConeTwistTwistSpanLimit,
			PhysicConstraintFactorIdx_ConeTwistSoftness, PhysicConstraintFactorIdx_ConeTwistBiasFactor, PhysicConstraintFactorIdx_ConeTwistRelaxationFactor,
			PhysicConstraintFactorIdx_LinearERP, PhysicConstraintFactorIdx_LinearCFM, PhysicConstraintFactorIdx_AngularERP, PhysicConstraintFactorIdx_AngularCFM,
		};
	case PhysicConstraint_Hinge:
		return {
			PhysicConstraintFactorIdx_HingeLowLimit, PhysicConstraintFactorIdx_HingeHighLimit, PhysicConstraintFactorIdx_HingeSoftness,
			PhysicConstraintFactorIdx_HingeBiasFactor, PhysicConstraintFactorIdx_HingeRelaxationFactor,
			PhysicConstraintFactorIdx_AngularERP, PhysicConstraintFactorIdx_AngularCFM, PhysicConstraintFactorIdx_AngularStopERP, PhysicConstraintFactorIdx_AngularStopCFM,
		};
	case PhysicConstraint_Point:
		return { PhysicConstraintFactorIdx_AngularERP, PhysicConstraintFactorIdx_AngularCFM };
	case PhysicConstraint_Slider:
		return {
			PhysicConstraintFactorIdx_SliderLowerLinearLimit, PhysicConstraintFactorIdx_SliderUpperLinearLimit,
			PhysicConstraintFactorIdx_SliderLowerAngularLimit, PhysicConstraintFactorIdx_SliderUpperAngularLimit,
			PhysicConstraintFactorIdx_LinearCFM, PhysicConstraintFactorIdx_LinearStopERP, PhysicConstraintFactorIdx_LinearStopCFM,
			PhysicConstraintFactorIdx_AngularCFM, PhysicConstraintFactorIdx_AngularStopERP, PhysicConstraintFactorIdx_AngularStopCFM,
			PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset,
		};
	case PhysicConstraint_Dof6:
		return dof6Factors;
	case PhysicConstraint_Dof6Spring:
	{
		auto factors = dof6Factors;
		for (int index = PhysicConstraintFactorIdx_Dof6SpringEnableLinearSpringX; index <= PhysicConstraintFactorIdx_Dof6SpringAngularDampingZ; ++index)
			factors.insert(index);
		return factors;
	}
	case PhysicConstraint_Fixed:
		return {
			PhysicConstraintFactorIdx_LinearCFM, PhysicConstraintFactorIdx_LinearStopERP, PhysicConstraintFactorIdx_LinearStopCFM,
			PhysicConstraintFactorIdx_AngularCFM, PhysicConstraintFactorIdx_AngularStopERP, PhysicConstraintFactorIdx_AngularStopCFM,
		};
	}
	return {};
}

// Factors that the *_physics.txt format stores for each behavior type.
static std::set<int> GetPersistedBehaviorFactors(int type)
{
	switch (type)
	{
	case PhysicBehavior_BarnacleDragOnRigidBody:
		return { PhysicBehaviorFactorIdx_BarnacleDragMagnitude, PhysicBehaviorFactorIdx_BarnacleDragExtraHeight };
	case PhysicBehavior_BarnacleDragOnConstraint:
		return {
			PhysicBehaviorFactorIdx_BarnacleDragMagnitude, PhysicBehaviorFactorIdx_BarnacleDragVelocity, PhysicBehaviorFactorIdx_BarnacleDragExtraHeight,
			PhysicBehaviorFactorIdx_BarnacleDragLimitAxis, PhysicBehaviorFactorIdx_BarnacleDragCalculateLimitFromActualPlayerOrigin,
			PhysicBehaviorFactorIdx_BarnacleDragUseServoMotor, PhysicBehaviorFactorIdx_BarnacleDragActivatedOnBarnaclePulling,
			PhysicBehaviorFactorIdx_BarnacleDragActivatedOnBarnacleChewing,
		};
	case PhysicBehavior_BarnacleChew:
		return { PhysicBehaviorFactorIdx_BarnacleChewMagnitude, PhysicBehaviorFactorIdx_BarnacleChewInterval };
	case PhysicBehavior_BarnacleConstraintLimitAdjustment:
		return {
			PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentExtraHeight, PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentInterval,
			PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentAxis,
		};
	case PhysicBehavior_GargantuaDragOnConstraint:
		return {
			PhysicBehaviorFactorIdx_BarnacleDragMagnitude, PhysicBehaviorFactorIdx_BarnacleDragVelocity, PhysicBehaviorFactorIdx_BarnacleDragExtraHeight,
			PhysicBehaviorFactorIdx_BarnacleDragLimitAxis, PhysicBehaviorFactorIdx_BarnacleDragUseServoMotor,
		};
	case PhysicBehavior_FirstPersonViewCamera:
	case PhysicBehavior_ThirdPersonViewCamera:
	{
		std::set<int> factors;
		for (int index = PhysicBehaviorFactorIdx_CameraActivateOnIdle; index <= PhysicBehaviorFactorIdx_CameraNewViewHeightDucking; ++index)
			factors.insert(index);
		return factors;
	}
	case PhysicBehavior_SimpleBuoyancy:
		return { PhysicBehaviorFactorIdx_SimpleBuoyancyMagnitude, PhysicBehaviorFactorIdx_SimpleBuoyancyLinearDamping, PhysicBehaviorFactorIdx_SimpleBuoyancyAngularDamping };
	}
	return {};
}

static std::shared_ptr<CClientCollisionShapeConfig> CreateShape(int type, float x, float y, float z)
{
	auto pShape = std::make_shared<CClientCollisionShapeConfig>();
	pShape->type = type;
	SetVector(pShape->size, x, y, z);
	return pShape;
}

static std::shared_ptr<CClientRigidBodyConfig> CreateRigidBody(const char* name, const std::shared_ptr<CClientCollisionShapeConfig>& pShape)
{
	auto pRigidBody = std::make_shared<CClientRigidBodyConfig>();
	pRigidBody->name = name;
	pRigidBody->collisionShape = pShape;
	return pRigidBody;
}

static void CheckSameCollisionShape(const CClientCollisionShapeConfig* pExpected, const CClientCollisionShapeConfig* pActual)
{
	assert(pActual);
	assert(pExpected->type == pActual->type);
	assert(pExpected->direction == pActual->direction);
	assert(pExpected->is_child == pActual->is_child);
	assert(IsNearVector(pExpected->size, pActual->size));
	assert(IsNearVector(pExpected->origin, pActual->origin));
	assert(IsNearVector(pExpected->angles, pActual->angles));
	assert(pExpected->resourcePath == pActual->resourcePath);
	assert(pExpected->compoundShapes.size() == pActual->compoundShapes.size());
	for (size_t i = 0; i < pExpected->compoundShapes.size(); ++i)
		CheckSameCollisionShape(pExpected->compoundShapes[i].get(), pActual->compoundShapes[i].get());
}

static void CheckSameRigidBody(const CClientRigidBodyConfig* pExpected, const CClientRigidBodyConfig* pActual)
{
	assert(pExpected->name == pActual->name);
	assert(pExpected->flags == pActual->flags);
	assert(pExpected->debugDrawLevel == pActual->debugDrawLevel);
	assert(pExpected->boneindex == pActual->boneindex);
	assert(IsNearVector(pExpected->origin, pActual->origin));
	assert(IsNearVector(pExpected->angles, pActual->angles));
	assert(IsNearVector(pExpected->forward, pActual->forward));
	assert(pExpected->isLegacyConfig == pActual->isLegacyConfig);
	assert(pExpected->pboneindex == pActual->pboneindex);
	assert(IsNear(pExpected->pboneoffset, pActual->pboneoffset));
	assert(IsNear(pExpected->mass, pActual->mass));
	assert(IsNear(pExpected->density, pActual->density));
	assert(IsNear(pExpected->linearFriction, pActual->linearFriction));
	assert(IsNear(pExpected->rollingFriction, pActual->rollingFriction));
	assert(IsNear(pExpected->restitution, pActual->restitution));
	assert(IsNear(pExpected->ccdRadius, pActual->ccdRadius));
	assert(IsNear(pExpected->ccdThreshold, pActual->ccdThreshold));
	assert(IsNear(pExpected->linearSleepingThreshold, pActual->linearSleepingThreshold));
	assert(IsNear(pExpected->angularSleepingThreshold, pActual->angularSleepingThreshold));
	assert(IsNear(pExpected->additionalDampingFactor, pActual->additionalDampingFactor));
	assert(IsNear(pExpected->additionalLinearDampingThresholdSqr, pActual->additionalLinearDampingThresholdSqr));
	assert(IsNear(pExpected->additionalAngularDampingThresholdSqr, pActual->additionalAngularDampingThresholdSqr));
	assert(!pExpected->collisionShape == !pActual->collisionShape);
	if (pExpected->collisionShape)
		CheckSameCollisionShape(pExpected->collisionShape.get(), pActual->collisionShape.get());
}

static void CheckSameConstraint(const CClientConstraintConfig* pExpected, const CClientConstraintConfig* pActual)
{
	assert(pExpected->name == pActual->name);
	assert(pExpected->type == pActual->type);
	assert(pExpected->rigidbodyA == pActual->rigidbodyA);
	assert(pExpected->rigidbodyB == pActual->rigidbodyB);
	assert(IsNearVector(pExpected->originA, pActual->originA));
	assert(IsNearVector(pExpected->anglesA, pActual->anglesA));
	assert(IsNearVector(pExpected->originB, pActual->originB));
	assert(IsNearVector(pExpected->anglesB, pActual->anglesB));
	assert(IsNearVector(pExpected->forward, pActual->forward));
	assert(pExpected->disableCollision == pActual->disableCollision);
	assert(pExpected->useGlobalJointFromA == pActual->useGlobalJointFromA);
	assert(pExpected->useLinearReferenceFrameA == pActual->useLinearReferenceFrameA);
	assert(pExpected->useLookAtOther == pActual->useLookAtOther);
	assert(pExpected->useGlobalJointOriginFromOther == pActual->useGlobalJointOriginFromOther);
	assert(pExpected->useRigidBodyDistanceAsLinearLimit == pActual->useRigidBodyDistanceAsLinearLimit);
	assert(pExpected->useSeperateLocalFrame == pActual->useSeperateLocalFrame);
	assert(pExpected->rotOrder == pActual->rotOrder);
	assert(pExpected->flags == pActual->flags);
	assert(pExpected->debugDrawLevel == pActual->debugDrawLevel);
	assert(IsNear(pExpected->maxTolerantLinearError, pActual->maxTolerantLinearError));
	assert(pExpected->isLegacyConfig == pActual->isLegacyConfig);
	assert(pExpected->boneindexA == pActual->boneindexA);
	assert(pExpected->boneindexB == pActual->boneindexB);
	assert(IsNearVector(pExpected->offsetA, pActual->offsetA));
	assert(IsNearVector(pExpected->offsetB, pActual->offsetB));
}

static void CheckSamePhysicBehavior(const CClientPhysicBehaviorConfig* pExpected, const CClientPhysicBehaviorConfig* pActual)
{
	assert(pExpected->name == pActual->name);
	assert(pExpected->type == pActual->type);
	assert(pExpected->rigidbodyA == pActual->rigidbodyA);
	assert(pExpected->rigidbodyB == pActual->rigidbodyB);
	assert(pExpected->constraint == pActual->constraint);
	assert(pExpected->flags == pActual->flags);
	assert(pExpected->debugDrawLevel == pActual->debugDrawLevel);
	assert(IsNearVector(pExpected->origin, pActual->origin));
	assert(IsNearVector(pExpected->angles, pActual->angles));
}

static void CheckSameAnimControl(const CClientAnimControlConfig* pExpected, const CClientAnimControlConfig* pActual)
{
	assert(pExpected->sequence == pActual->sequence);
	assert(pExpected->gaitsequence == pActual->gaitsequence);
	assert(IsNear(pExpected->animframe, pActual->animframe));
	assert(pExpected->activityType == pActual->activityType);
	assert(pExpected->flags == pActual->flags);
	for (int i = 0; i < _ARRAYSIZE(pExpected->controller); ++i)
	{
		assert(pExpected->controller[i] == pActual->controller[i]);
		assert(pExpected->blending[i] == pActual->blending[i]);
	}
}

static void CheckSameObjectHeader(const CClientPhysicObjectConfig* pExpected, const CClientPhysicObjectConfig* pActual)
{
	assert(pExpected->type == pActual->type);
	// Every configuration loaded from a file is saveable again.
	assert((pExpected->flags | PhysicObjectFlag_FromConfig) == pActual->flags);
	assert(pExpected->verifyBoneChunk == pActual->verifyBoneChunk);
	assert(pExpected->verifyModelFile == pActual->verifyModelFile);
	assert(pExpected->crc32BoneChunk == pActual->crc32BoneChunk);
	assert(pExpected->crc32ModelFile == pActual->crc32ModelFile);
}

static void CheckRegistered(const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	// The editor resolves configurations by configId.
	assert(pPhysicObjectConfig == UTIL_GetPhysicObjectConfigFromConfigId(pPhysicObjectConfig->configId).get());
	for (const auto& pRigidBody : pPhysicObjectConfig->RigidBodyConfigs)
	{
		assert(pRigidBody == UTIL_GetRigidConfigFromConfigId(pRigidBody->configId));
		if (pRigidBody->collisionShape)
			assert(pRigidBody->collisionShape == ClientPhysicManager()->GetPhysicConfig(pRigidBody->collisionShape->configId).lock());
	}
	for (const auto& pConstraint : pPhysicObjectConfig->ConstraintConfigs)
		assert(pConstraint == UTIL_GetConstraintConfigFromConfigId(pConstraint->configId));
	for (const auto& pPhysicBehavior : pPhysicObjectConfig->PhysicBehaviorConfigs)
		assert(pPhysicBehavior == UTIL_GetPhysicBehaviorConfigFromConfigId(pPhysicBehavior->configId));
}

static std::shared_ptr<CClientPhysicObjectConfig> SaveAndReload(const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
	auto text = TestSavePhysicObjectConfigToText(pPhysicObjectConfig);
	auto pReloaded = TestLoadPhysicObjectConfigFromText(text, TestGetModelWithoutStudioData());
	assert(pReloaded);
	CheckRegistered(pReloaded.get());
	return pReloaded;
}

static void TestTypeNamesRoundTrip()
{
	// These names are the on-disk format; each one has to parse back to the same type.
	for (int type = PhysicShape_None + 1; type < PhysicShape_Maximum; ++type)
		assert(type == UTIL_GetCollisionTypeFromTypeName(UTIL_GetCollisionShapeTypeName(type)));
	for (int type = PhysicConstraint_None + 1; type < PhysicConstraint_Maximum; ++type)
		assert(type == UTIL_GetConstraintTypeFromTypeName(UTIL_GetConstraintTypeName(type)));
	for (int type = PhysicBehavior_None + 1; type < PhysicBehavior_Maximum; ++type)
		assert(type == UTIL_GetPhysicBehaviorTypeFromTypeName(UTIL_GetPhysicBehaviorTypeName(type)));

	assert(PhysicShape_None == UTIL_GetCollisionTypeFromTypeName("Unknown"));
	assert(PhysicConstraint_None == UTIL_GetConstraintTypeFromTypeName("Unknown"));
	assert(PhysicBehavior_None == UTIL_GetPhysicBehaviorTypeFromTypeName("Unknown"));
}

static void TestRagdollConfigRoundTrip()
{
	auto pConfig = std::make_shared<CClientRagdollObjectConfig>();
	pConfig->flags |= PhysicObjectFlag_Barnacle | PhysicObjectFlag_Gargantua | PhysicObjectFlag_OverrideStudioCheckBBox;
	pConfig->verifyBoneChunk = true;
	pConfig->crc32BoneChunk = "1a2b3c4d";
	pConfig->verifyModelFile = true;
	pConfig->crc32ModelFile = "deadbeef";

	auto pBoxChild = CreateShape(PhysicShape_Box, 2, 3, 4);
	pBoxChild->is_child = true;
	SetVector(pBoxChild->origin, 1, 2, 3);
	SetVector(pBoxChild->angles, 10, 20, 30);
	auto pMeshChild = CreateShape(PhysicShape_TriangleMesh, 0, 0, 0);
	pMeshChild->is_child = true;
	pMeshChild->direction = PhysicShapeDirection_Z;
	pMeshChild->resourcePath = "models/test_mesh.obj";
	auto pCompound = CreateShape(PhysicShape_Compound, 0, 0, 0);
	pCompound->compoundShapes = { pBoxChild, pMeshChild };

	auto pPelvis = CreateRigidBody("Pelvis", pCompound);
	pPelvis->flags = PhysicRigidBodyFlag_AllowedOnRagdollObject;
	pPelvis->debugDrawLevel = 3;
	pPelvis->boneindex = 4;
	SetVector(pPelvis->origin, 1.5f, -2, 3);
	SetVector(pPelvis->angles, 10, 20, 30);
	SetVector(pPelvis->forward, 1, 0, 0);
	pPelvis->mass = 2.5f;
	pPelvis->density = 0.75f;
	pPelvis->linearFriction = 0.5f;
	pPelvis->rollingFriction = 0.125f;
	pPelvis->restitution = 0.25f;
	pPelvis->ccdRadius = 1.5f;
	pPelvis->ccdThreshold = 0.0625f;
	pPelvis->linearSleepingThreshold = 2.5f;
	pPelvis->angularSleepingThreshold = 1.25f;
	pPelvis->additionalDampingFactor = 0.375f;
	pPelvis->additionalLinearDampingThresholdSqr = 2;
	pPelvis->additionalAngularDampingThresholdSqr = 0.5f;

	auto pCapsule = CreateShape(PhysicShape_Capsule, 5, 6, 0);
	pCapsule->direction = PhysicShapeDirection_X;
	auto pHead = CreateRigidBody("Head", pCapsule);
	pHead->boneindex = 7;
	pHead->isLegacyConfig = true;
	pHead->pboneindex = 2;
	pHead->pboneoffset = 1.25f;
	pConfig->RigidBodyConfigs = { pPelvis, pHead };

	for (int type = PhysicConstraint_None + 1; type < PhysicConstraint_Maximum; ++type)
	{
		auto pConstraint = std::make_shared<CClientConstraintConfig>();
		pConstraint->name = std::string("Joint") + UTIL_GetConstraintTypeName(type);
		pConstraint->type = type;
		pConstraint->rigidbodyA = "Pelvis";
		pConstraint->rigidbodyB = "Head";
		SetVector(pConstraint->originA, 1, 2, 3);
		SetVector(pConstraint->anglesA, 0, 90, 0);
		SetVector(pConstraint->originB, -1, -2, -3);
		SetVector(pConstraint->anglesB, 45, 0, 0);
		SetVector(pConstraint->forward, 0, 0, 1);
		pConstraint->disableCollision = false;
		pConstraint->useGlobalJointFromA = false;
		pConstraint->useLinearReferenceFrameA = false;
		pConstraint->useLookAtOther = true;
		pConstraint->useGlobalJointOriginFromOther = true;
		pConstraint->useRigidBodyDistanceAsLinearLimit = true;
		pConstraint->useSeperateLocalFrame = true;
		pConstraint->rotOrder = PhysicRotOrder_ZYX;
		pConstraint->flags = PhysicConstraintFlag_Barnacle | PhysicConstraintFlag_Gargantua | PhysicConstraintFlag_DeactiveOnNormalActivity |
			PhysicConstraintFlag_DeactiveOnDeathActivity | PhysicConstraintFlag_DeactiveOnCaughtByBarnacleActivity |
			PhysicConstraintFlag_DeactiveOnBarnaclePullingActivity | PhysicConstraintFlag_DeactiveOnBarnacleChewingActivity |
			PhysicConstraintFlag_DeactiveOnGargantuaBiteActivity | PhysicConstraintFlag_DontResetPoseOnErrorCorrection | PhysicConstraintFlag_DeferredCreate;
		pConstraint->debugDrawLevel = 2;
		pConstraint->maxTolerantLinearError = 12.5f;
		pConstraint->isLegacyConfig = true;
		pConstraint->boneindexA = 3;
		pConstraint->boneindexB = 5;
		SetVector(pConstraint->offsetA, 0, 0, 4);
		SetVector(pConstraint->offsetB, 0, -4, 0);
		for (int index = 0; index < PhysicConstraintFactorIdx_Maximum; ++index)
			pConstraint->factors[index] = 0.0625f * (index + 1);
		// A factor left out of the file must stay "not provided".
		pConstraint->factors[PhysicConstraintFactorIdx_AngularCFM] = NAN;
		pConfig->ConstraintConfigs.emplace_back(pConstraint);
	}

	for (int type = PhysicBehavior_None + 1; type < PhysicBehavior_Maximum; ++type)
	{
		auto pPhysicBehavior = std::make_shared<CClientPhysicBehaviorConfig>();
		pPhysicBehavior->name = std::string("Behavior") + UTIL_GetPhysicBehaviorTypeName(type);
		pPhysicBehavior->type = type;
		pPhysicBehavior->rigidbodyA = "Pelvis";
		pPhysicBehavior->rigidbodyB = "Head";
		pPhysicBehavior->constraint = "JointHinge";
		pPhysicBehavior->flags = PhysicBehaviorFlag_Barnacle | PhysicBehaviorFlag_Gargantua;
		pPhysicBehavior->debugDrawLevel = 3;
		SetVector(pPhysicBehavior->origin, 4, 5, 6);
		SetVector(pPhysicBehavior->angles, -10, 0, 10);
		for (int index = 0; index < PhysicBehaviorFactorIdx_Maximum; ++index)
			pPhysicBehavior->factors[index] = 0.25f * (index + 1);
		pConfig->PhysicBehaviorConfigs.emplace_back(pPhysicBehavior);
	}

	auto pDeathAnim = std::make_shared<CClientAnimControlConfig>();
	pDeathAnim->sequence = 15;
	pDeathAnim->gaitsequence = 3;
	pDeathAnim->animframe = 80;
	pDeathAnim->activityType = StudioAnimActivityType_Death;
	pDeathAnim->flags = AnimControlFlag_OverrideController | AnimControlFlag_OverrideBlending;
	for (int i = 0; i < _ARRAYSIZE(pDeathAnim->controller); ++i)
	{
		pDeathAnim->controller[i] = 10 + i;
		pDeathAnim->blending[i] = 20 + i;
	}
	auto pIdleAnim = std::make_shared<CClientAnimControlConfig>();
	pIdleAnim->sequence = 1;
	pIdleAnim->flags = AnimControlFlag_OverrideAllBones;
	pConfig->AnimControlConfigs = { pDeathAnim, pIdleAnim };

	auto pReloaded = UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(SaveAndReload(pConfig.get()));
	assert(pReloaded);
	CheckSameObjectHeader(pConfig.get(), pReloaded.get());

	assert(pConfig->RigidBodyConfigs.size() == pReloaded->RigidBodyConfigs.size());
	for (size_t i = 0; i < pConfig->RigidBodyConfigs.size(); ++i)
		CheckSameRigidBody(pConfig->RigidBodyConfigs[i].get(), pReloaded->RigidBodyConfigs[i].get());

	assert(pConfig->ConstraintConfigs.size() == pReloaded->ConstraintConfigs.size());
	for (size_t i = 0; i < pConfig->ConstraintConfigs.size(); ++i)
	{
		const auto& pExpected = pConfig->ConstraintConfigs[i];
		const auto& pActual = pReloaded->ConstraintConfigs[i];
		CheckSameConstraint(pExpected.get(), pActual.get());
		// Only the factors that belong to the constraint type are stored.
		const auto persisted = GetPersistedConstraintFactors(pExpected->type);
		for (int index = 0; index < PhysicConstraintFactorIdx_Maximum; ++index)
			assert(IsSameFactor(persisted.contains(index) ? pExpected->factors[index] : NAN, pActual->factors[index]));
	}

	assert(pConfig->PhysicBehaviorConfigs.size() == pReloaded->PhysicBehaviorConfigs.size());
	for (size_t i = 0; i < pConfig->PhysicBehaviorConfigs.size(); ++i)
	{
		const auto& pExpected = pConfig->PhysicBehaviorConfigs[i];
		const auto& pActual = pReloaded->PhysicBehaviorConfigs[i];
		CheckSamePhysicBehavior(pExpected.get(), pActual.get());
		const auto persisted = GetPersistedBehaviorFactors(pExpected->type);
		for (int index = 0; index < PhysicBehaviorFactorIdx_Maximum; ++index)
			assert(IsSameFactor(persisted.contains(index) ? pExpected->factors[index] : NAN, pActual->factors[index]));
	}

	assert(pConfig->AnimControlConfigs.size() == pReloaded->AnimControlConfigs.size());
	for (size_t i = 0; i < pConfig->AnimControlConfigs.size(); ++i)
	{
		CheckSameAnimControl(pConfig->AnimControlConfigs[i].get(), pReloaded->AnimControlConfigs[i].get());
		assert(pReloaded->AnimControlConfigs[i] == UTIL_GetAnimControlConfigFromConfigId(pReloaded->AnimControlConfigs[i]->configId));
	}
}

static void TestStaticAndDynamicConfigRoundTrip()
{
	auto pMesh = CreateShape(PhysicShape_TriangleMesh, 0, 0, 0);
	pMesh->resourcePath = "models/barnacle.obj";
	auto pStaticConfig = std::make_shared<CClientStaticObjectConfig>();
	pStaticConfig->RigidBodyConfigs = { CreateRigidBody("Body", pMesh) };
	pStaticConfig->RigidBodyConfigs[0]->flags = PhysicRigidBodyFlag_AlwaysStatic | PhysicRigidBodyFlag_NoCollisionToWorld;

	auto pReloadedStatic = SaveAndReload(pStaticConfig.get());
	CheckSameObjectHeader(pStaticConfig.get(), pReloadedStatic.get());
	assert(1 == pReloadedStatic->RigidBodyConfigs.size());
	CheckSameRigidBody(pStaticConfig->RigidBodyConfigs[0].get(), pReloadedStatic->RigidBodyConfigs[0].get());

	auto pDynamicConfig = std::make_shared<CClientDynamicObjectConfig>();
	pDynamicConfig->RigidBodyConfigs = { CreateRigidBody("Door", CreateShape(PhysicShape_Box, 32, 4, 48)), CreateRigidBody("Frame", CreateShape(PhysicShape_Cylinder, 2, 48, 2)) };
	pDynamicConfig->RigidBodyConfigs[0]->flags = PhysicRigidBodyFlag_AlwaysDynamic;
	auto pHinge = std::make_shared<CClientConstraintConfig>();
	pHinge->name = "DoorHinge";
	pHinge->type = PhysicConstraint_Hinge;
	pHinge->rigidbodyA = "Frame";
	pHinge->rigidbodyB = "Door";
	pHinge->factors[PhysicConstraintFactorIdx_HingeLowLimit] = -0.5f;
	pHinge->factors[PhysicConstraintFactorIdx_HingeHighLimit] = 0.5f;
	pDynamicConfig->ConstraintConfigs = { pHinge };

	auto pReloadedDynamic = SaveAndReload(pDynamicConfig.get());
	CheckSameObjectHeader(pDynamicConfig.get(), pReloadedDynamic.get());
	assert(2 == pReloadedDynamic->RigidBodyConfigs.size());
	for (size_t i = 0; i < pDynamicConfig->RigidBodyConfigs.size(); ++i)
		CheckSameRigidBody(pDynamicConfig->RigidBodyConfigs[i].get(), pReloadedDynamic->RigidBodyConfigs[i].get());
	assert(1 == pReloadedDynamic->ConstraintConfigs.size());
	CheckSameConstraint(pHinge.get(), pReloadedDynamic->ConstraintConfigs[0].get());
	assert(IsSameFactor(-0.5f, pReloadedDynamic->ConstraintConfigs[0]->factors[PhysicConstraintFactorIdx_HingeLowLimit]));
	assert(IsSameFactor(0.5f, pReloadedDynamic->ConstraintConfigs[0]->factors[PhysicConstraintFactorIdx_HingeHighLimit]));
}

static std::string MakeConfigWithEveryGroup(const char* type)
{
	return std::string(R"("PhysicObjectConfig"
{
	"type"	")") + type + R"("
	"rigidBodies"
	{
		"Body"
		{
			"alwaysDynamic"	"1"
			"alwaysStatic"	"1"
			"invertStateOnDeath"	"1"
			"noCollisionToWorld"	"1"
			"collisionShape"
			{
				"type"	"Box"
				"size"	"1 2 3"
			}
		}
	}
	"constraints"
	{
		"Joint"
		{
			"type"	"Point"
			"rigidbodyA"	"Body"
			"rigidbodyB"	"Body"
		}
	}
	"physicBehaviors"
	{
		"Float"
		{
			"type"	"SimpleBuoyancy"
			"rigidbodyA"	"Body"
		}
	}
	"animControls"
	{
		"0"
		{
			"sequence"	"4"
		}
	}
}
)";
}

static void TestObjectTypeSelectsLoadedGroups()
{
	// Each object type reads only its own groups and only the rigid-body flags it supports.
	auto pStatic = TestLoadPhysicObjectConfigFromText(MakeConfigWithEveryGroup("StaticObject"), TestGetModelWithoutStudioData());
	assert(pStatic && PhysicObjectType_StaticObject == pStatic->type);
	assert(1 == pStatic->RigidBodyConfigs.size());
	assert((PhysicRigidBodyFlag_AlwaysStatic | PhysicRigidBodyFlag_NoCollisionToWorld) == pStatic->RigidBodyConfigs[0]->flags);
	assert(pStatic->ConstraintConfigs.empty());
	assert(pStatic->PhysicBehaviorConfigs.empty());

	auto pDynamic = TestLoadPhysicObjectConfigFromText(MakeConfigWithEveryGroup("DynamicObject"), TestGetModelWithoutStudioData());
	assert(pDynamic && PhysicObjectType_DynamicObject == pDynamic->type);
	assert((PhysicRigidBodyFlag_AlwaysDynamic | PhysicRigidBodyFlag_NoCollisionToWorld) == pDynamic->RigidBodyConfigs[0]->flags);
	assert(1 == pDynamic->ConstraintConfigs.size());
	assert(pDynamic->PhysicBehaviorConfigs.empty());

	auto pRagdoll = UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(
		TestLoadPhysicObjectConfigFromText(MakeConfigWithEveryGroup("RagdollObject"), TestGetModelWithoutStudioData()));
	assert(pRagdoll);
	assert((PhysicRigidBodyFlag_AlwaysDynamic | PhysicRigidBodyFlag_InvertStateOnDeath | PhysicRigidBodyFlag_NoCollisionToWorld) == pRagdoll->RigidBodyConfigs[0]->flags);
	assert(1 == pRagdoll->ConstraintConfigs.size());
	assert(1 == pRagdoll->PhysicBehaviorConfigs.size());
	assert(1 == pRagdoll->AnimControlConfigs.size());
	assert(4 == pRagdoll->AnimControlConfigs[0]->sequence);
}

static void TestLoaderAppliesDocumentedDefaults()
{
	auto pConfig = UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(TestLoadPhysicObjectConfigFromText(R"("PhysicObjectConfig"
{
	"type"	"RagdollObject"
	"rigidBodies"
	{
		"Body"
		{
			"collisionShape"
			{
				"type"	"Sphere"
				"size"	"4"
			}
		}
	}
	"constraints"
	{
		"Joint"
		{
			"type"	"Hinge"
		}
	}
	"physicBehaviors"
	{
		"Camera"
		{
			"type"	"FirstPersonViewCamera"
		}
	}
	"animControls"
	{
		"0"
		{
		}
	}
}
)", TestGetModelWithoutStudioData()));
	assert(pConfig);
	assert((PhysicObjectFlag_RagdollObject | PhysicObjectFlag_FromConfig) == pConfig->flags);
	assert(!pConfig->verifyBoneChunk && !pConfig->verifyModelFile);

	const auto& pRigidBody = pConfig->RigidBodyConfigs[0];
	assert(PhysicRigidBodyFlag_None == pRigidBody->flags);
	assert(BULLET_DEFAULT_DEBUG_DRAW_LEVEL == pRigidBody->debugDrawLevel);
	assert(-1 == pRigidBody->boneindex);
	assert(-1 == pRigidBody->pboneindex);
	assert(!pRigidBody->isLegacyConfig);
	const vec3_t defaultForward = { 0, 1, 0 };
	assert(IsNearVector(defaultForward, pRigidBody->forward));
	assert(IsNear(BULLET_DEFAULT_MASS, pRigidBody->mass));
	assert(IsNear(BULLET_DEFAULT_DENSENTY, pRigidBody->density));
	assert(IsNear(BULLET_DEFAULT_LINEAR_FRICTION, pRigidBody->linearFriction));
	assert(IsNear(BULLET_DEFAULT_ANGULAR_FRICTION, pRigidBody->rollingFriction));
	assert(IsNear(BULLET_DEFAULT_RESTITUTION, pRigidBody->restitution));
	assert(IsNear(0, pRigidBody->ccdRadius));
	assert(IsNear(BULLET_DEFAULT_CCD_THRESHOLD, pRigidBody->ccdThreshold));
	assert(IsNear(BULLET_DEFAULT_LINEAR_SLEEPING_THRESHOLD, pRigidBody->linearSleepingThreshold));
	assert(IsNear(BULLET_DEFAULT_ANGULAR_SLEEPING_THRESHOLD, pRigidBody->angularSleepingThreshold));
	assert(IsNear(BULLET_DEFAULT_ADDITIONAL_DAMPING_FACTOR, pRigidBody->additionalDampingFactor));
	assert(IsNear(BULLET_DEFAULT_ADDITIONAL_LINEAR_DAMPING_THRESHOLD_SQR, pRigidBody->additionalLinearDampingThresholdSqr));
	assert(IsNear(BULLET_DEFAULT_ADDITIONAL_ANGULAR_DAMPING_THRESHOLD_SQR, pRigidBody->additionalAngularDampingThresholdSqr));
	// "size" accepts one, two or three components.
	assert(PhysicShape_Sphere == pRigidBody->collisionShape->type);
	assert(PhysicShapeDirection_Y == pRigidBody->collisionShape->direction);
	assert(IsNear(4, pRigidBody->collisionShape->size[0]));

	const auto& pConstraint = pConfig->ConstraintConfigs[0];
	assert(pConstraint->disableCollision);
	assert(pConstraint->useGlobalJointFromA);
	assert(pConstraint->useLinearReferenceFrameA);
	assert(!pConstraint->useLookAtOther);
	assert(!pConstraint->useGlobalJointOriginFromOther);
	assert(!pConstraint->useRigidBodyDistanceAsLinearLimit);
	assert(!pConstraint->useSeperateLocalFrame);
	assert(0 == pConstraint->flags);
	assert(BULLET_DEFAULT_DEBUG_DRAW_LEVEL == pConstraint->debugDrawLevel);
	assert(IsNear(BULLET_DEFAULT_MAX_TOLERANT_LINEAR_ERROR, pConstraint->maxTolerantLinearError));
	assert(-1 == pConstraint->boneindexA && -1 == pConstraint->boneindexB);
	// Missing factors stay NaN so the Bullet backend applies its own defaults.
	for (int index = 0; index < PhysicConstraintFactorIdx_Maximum; ++index)
		assert(std::isnan(pConstraint->factors[index]));

	const auto& pPhysicBehavior = pConfig->PhysicBehaviorConfigs[0];
	assert(PhysicBehavior_FirstPersonViewCamera == pPhysicBehavior->type);
	for (int index = 0; index < PhysicBehaviorFactorIdx_Maximum; ++index)
		assert(std::isnan(pPhysicBehavior->factors[index]));

	const auto& pAnimControl = pConfig->AnimControlConfigs[0];
	assert(-1 == pAnimControl->sequence && -1 == pAnimControl->gaitsequence);
	assert(StudioAnimActivityType_Idle == pAnimControl->activityType);
	for (int i = 0; i < _ARRAYSIZE(pAnimControl->controller); ++i)
		assert(-1 == pAnimControl->controller[i] && -1 == pAnimControl->blending[i]);
}

static void TestLoaderRejectsUnknownObjectTypes()
{
	TestClearConsoleMessage();
	assert(nullptr == TestLoadPhysicObjectConfigFromText(R"("PhysicObjectConfig" { "type" "SoftBody" })", TestGetModelWithoutStudioData()));
	assert(Contains(TestGetLastConsoleMessage(), "SoftBody"));

	TestClearConsoleMessage();
	assert(nullptr == TestLoadPhysicObjectConfigFromText(R"("PhysicObjectConfig" { "rigidBodies" { } })", TestGetModelWithoutStudioData()));
	assert(!TestGetLastConsoleMessage().empty());
}

struct CTestStudioModel
{
	studiohdr_t header;
	mstudiobone_t bones[2];
};

static void* TestModExtradata(model_t* mod)
{
	return mod->cache.data;
}

// Reference CRC-32 (IEEE 802.3), independent of the hash library used by the plugin.
static std::string ReferenceCrc32(const void* data, size_t size)
{
	uint32_t crc = 0xFFFFFFFFu;
	for (size_t i = 0; i < size; ++i)
	{
		crc ^= static_cast<const uint8_t*>(data)[i];
		for (int bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
	}
	char text[9];
	snprintf(text, sizeof(text), "%08x", crc ^ 0xFFFFFFFFu);
	return text;
}

static std::string MakeVerifiedConfig(const char* verifyKey, bool verify, const char* crcKey, const std::string& crc)
{
	return std::string(R"("PhysicObjectConfig"
{
	"type"	"RagdollObject"
	")") + verifyKey + "\"\t\"" + (verify ? "1" : "0") + "\"\n\t\"" + crcKey + "\"\t\"" + crc + "\"\n}\n";
}

static void TestIntegrityVerificationMatchesModel()
{
	CTestStudioModel studio{};
	studio.header.length = sizeof(studio);
	studio.header.numbones = _ARRAYSIZE(studio.bones);
	studio.header.boneindex = offsetof(CTestStudioModel, bones);
	strcpy(studio.bones[0].name, "Bip01");
	studio.bones[0].parent = -1;
	strcpy(studio.bones[1].name, "Bip01 Pelvis");
	studio.bones[1].parent = 0;

	model_t mod{};
	mod.type = mod_studio;
	mod.cache.data = &studio;
	IEngineStudio.Mod_Extradata = TestModExtradata;

	// The bone checksum covers exactly the skeleton; the model checksum covers the file data.
	std::string boneChunkCrc;
	std::string modelFileCrc;
	assert(UTIL_GetCrc32ForBoneChunk(&mod, &boneChunkCrc));
	assert(UTIL_GetCrc32ForModelFile(&mod, &modelFileCrc));
	assert(ReferenceCrc32(studio.bones, sizeof(studio.bones)) == boneChunkCrc);
	assert(ReferenceCrc32(&studio, sizeof(studio)) == modelFileCrc);

	auto boneVerified = MakeVerifiedConfig("verifyBoneChunk", true, "crc32BoneChunk", boneChunkCrc);
	auto modelVerified = MakeVerifiedConfig("verifyModelFile", true, "crc32ModelFile", modelFileCrc);
	assert(TestLoadPhysicObjectConfigFromText(boneVerified, &mod));
	assert(TestLoadPhysicObjectConfigFromText(modelVerified, &mod));

	// Model data outside the skeleton invalidates only the whole-file checksum.
	studio.header.flags = 1;
	assert(TestLoadPhysicObjectConfigFromText(boneVerified, &mod));
	TestClearConsoleMessage();
	assert(nullptr == TestLoadPhysicObjectConfigFromText(modelVerified, &mod));
	assert(Contains(TestGetLastConsoleMessage(), "crc32-modelfile"));

	// A changed skeleton rejects the bone-verified configuration.
	studio.bones[1].parent = -1;
	TestClearConsoleMessage();
	assert(nullptr == TestLoadPhysicObjectConfigFromText(boneVerified, &mod));
	assert(Contains(TestGetLastConsoleMessage(), "crc32-bonechunk"));

	// A stale checksum is ignored when verification is off.
	assert(TestLoadPhysicObjectConfigFromText(MakeVerifiedConfig("verifyBoneChunk", false, "crc32BoneChunk", boneChunkCrc), &mod));
}

static const char* const LegacyRagdollConfig =
	"// Legacy ragdoll configuration\r\n"
	"# comment\r\n"
	"[DeathAnim]\r\n"
	"15 80\r\n"
	"\r\n"
	"[RigidBody]\r\n"
	"Pelvis 1 8 sphere 2.0 8.0 0.0 10\r\n"
	"Spine 11 12 capsule 0.5 3.0 6.0 5\r\n"
	"\n"
	"[JiggleBone]\n"
	"Hair 40 39 capsule 1.0 2.0 6.0 0.5\n"
	"\n"
	"[Constraint]\n"
	"Pelvis Spine conetwist 1 11 0 0 4 0 -4 0 0.25 0.5 0.125\n"
	"Spine Hair hinge_collision 12 40 1 2 3 4 5 6 -0.5 0.25 0\n"
	"Pelvis Hair point 1 40 0 0 0 0 0 0 0 0 0\n"
	"\n"
	"[Barnacle]\n"
	"Pelvis dof6 0 0 8 1000 24 4\n"
	"Spine chewforce 0 0 0 5000 1.5 0\n"
	"Pelvis chewlimit 0 0 0 0 8 1\n"
	"\n"
	"[Gargantua]\n"
	"Pelvis Body dof6z 0 0 0 3000 16 0\n"
	"\n"
	"[WaterControl]\n"
	"Pelvis 0 0 0 1.5 0.25 0.75\n"
	"Pelvis 8 0 0 1.5 0.25 0.75\n"
	"Spine 0 0 2 0.5 0.25 0\n"
	"\n"
	"[CameraControl]\n"
	"FirstPerson_OriginOffset 1 2 3\n"
	"ThirdPerson_OriginOffset 4 5 6\n"
	"ThirdPerson_AngleOffset 0 90 0\n";

static const CClientConstraintConfig* FindConstraint(const CClientPhysicObjectConfig* pConfig, const char* name)
{
	for (const auto& pConstraint : pConfig->ConstraintConfigs)
		if (pConstraint->name == name)
			return pConstraint.get();
	return nullptr;
}

static const CClientPhysicBehaviorConfig* FindPhysicBehavior(const CClientPhysicObjectConfig* pConfig, const char* name)
{
	for (const auto& pPhysicBehavior : pConfig->PhysicBehaviorConfigs)
		if (pPhysicBehavior->name == name)
			return pPhysicBehavior.get();
	return nullptr;
}

static void TestLegacyRagdollFormat()
{
	auto pConfig = UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(LoadPhysicObjectConfigFromLegacyFileBuffer(LegacyRagdollConfig));
	assert(pConfig);
	assert((PhysicObjectFlag_RagdollObject | PhysicObjectFlag_FromConfig | PhysicObjectFlag_OverrideStudioCheckBBox) == pConfig->flags);
	CheckRegistered(pConfig.get());

	assert(1 == pConfig->AnimControlConfigs.size());
	assert(15 == pConfig->AnimControlConfigs[0]->sequence);
	assert(IsNear(80, pConfig->AnimControlConfigs[0]->animframe));
	assert(StudioAnimActivityType_Death == pConfig->AnimControlConfigs[0]->activityType);
	assert(AnimControlFlag_OverrideAllBones == pConfig->AnimControlConfigs[0]->flags);

	// name boneindex pboneindex shape pboneoffset size0 size1 mass
	assert(3 == pConfig->RigidBodyConfigs.size());
	const auto& pPelvis = pConfig->RigidBodyConfigs[0];
	assert("Pelvis" == pPelvis->name);
	assert(1 == pPelvis->boneindex && 8 == pPelvis->pboneindex);
	assert(IsNear(2, pPelvis->pboneoffset));
	assert(IsNear(10, pPelvis->mass));
	assert(pPelvis->isLegacyConfig);
	assert((PhysicRigidBodyFlag_InvertStateOnDeath | PhysicRigidBodyFlag_InvertStateOnCaughtByBarnacle) == pPelvis->flags);
	assert(PhysicShape_Sphere == pPelvis->collisionShape->type);
	assert(IsNear(8, pPelvis->collisionShape->size[0]));
	assert(IsNear(8 * 0.2f, pPelvis->ccdRadius));

	const auto& pSpine = pConfig->RigidBodyConfigs[1];
	assert(PhysicShape_Capsule == pSpine->collisionShape->type);
	assert(PhysicShapeDirection_Y == pSpine->collisionShape->direction);
	assert(IsNear(3, pSpine->collisionShape->size[0]) && IsNear(6, pSpine->collisionShape->size[1]));

	// Jiggle bones simulate even while the entity is alive.
	const auto& pHair = pConfig->RigidBodyConfigs[2];
	assert("Hair" == pHair->name);
	assert(PhysicRigidBodyFlag_AlwaysDynamic == pHair->flags);

	// rigidbodyA rigidbodyB type boneindexA boneindexB offsetA offsetB factor0 factor1 factor2
	auto pConeTwist = FindConstraint(pConfig.get(), "NativeConstraint|Pelvis|Spine");
	assert(pConeTwist && PhysicConstraint_ConeTwist == pConeTwist->type);
	assert("Pelvis" == pConeTwist->rigidbodyA && "Spine" == pConeTwist->rigidbodyB);
	assert(1 == pConeTwist->boneindexA && 11 == pConeTwist->boneindexB);
	const vec3_t offsetA = { 0, 0, 4 };
	const vec3_t offsetB = { 0, -4, 0 };
	assert(IsNearVector(offsetA, pConeTwist->offsetA) && IsNearVector(offsetB, pConeTwist->offsetB));
	assert(pConeTwist->isLegacyConfig && pConeTwist->disableCollision);
	assert(IsSameFactor(0.25f, pConeTwist->factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit1]));
	assert(IsSameFactor(0.5f, pConeTwist->factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit2]));
	assert(IsSameFactor(0.125f, pConeTwist->factors[PhysicConstraintFactorIdx_ConeTwistTwistSpanLimit]));

	auto pHinge = FindConstraint(pConfig.get(), "NativeConstraint|Spine|Hair");
	assert(pHinge && PhysicConstraint_Hinge == pHinge->type);
	assert(!pHinge->disableCollision);
	assert(IsSameFactor(-0.5f, pHinge->factors[PhysicConstraintFactorIdx_HingeLowLimit]));
	assert(IsSameFactor(0.25f, pHinge->factors[PhysicConstraintFactorIdx_HingeHighLimit]));

	auto pPoint = FindConstraint(pConfig.get(), "NativeConstraint|Pelvis|Hair");
	assert(pPoint && PhysicConstraint_Point == pPoint->type);

	auto pChew = FindPhysicBehavior(pConfig.get(), "BarnacleChew|Spine");
	assert(pChew && PhysicBehavior_BarnacleChew == pChew->type);
	assert("Spine" == pChew->rigidbodyA && PhysicBehaviorFlag_Barnacle == pChew->flags);
	assert(IsSameFactor(5000, pChew->factors[PhysicBehaviorFactorIdx_BarnacleChewMagnitude]));
	assert(IsSameFactor(1.5f, pChew->factors[PhysicBehaviorFactorIdx_BarnacleChewInterval]));

	auto pChewLimit = FindPhysicBehavior(pConfig.get(), "BarnacleConstraintLimitAdjustment|Pelvis");
	assert(pChewLimit && PhysicBehavior_BarnacleConstraintLimitAdjustment == pChewLimit->type);
	assert("BarnacleConstraint|Pelvis" == pChewLimit->constraint);
	assert(IsSameFactor(8, pChewLimit->factors[PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentExtraHeight]));
	assert(IsSameFactor(1, pChewLimit->factors[PhysicBehaviorFactorIdx_BarnacleConstraintLimitAdjustmentInterval]));

	// The gargantua constraint attaches to the gargantua object's rigid body at runtime.
	auto pGargConstraint = FindConstraint(pConfig.get(), "GargConstraint|Pelvis");
	assert(pGargConstraint && PhysicConstraint_Dof6Spring == pGargConstraint->type);
	assert("@gargantua.Body" == pGargConstraint->rigidbodyA && "Pelvis" == pGargConstraint->rigidbodyB);
	assert(PhysicConstraintFlag_Gargantua == pGargConstraint->flags);
	auto pGargDrag = FindPhysicBehavior(pConfig.get(), "GargantuaDragForce|Pelvis");
	assert(pGargDrag && PhysicBehavior_GargantuaDragOnConstraint == pGargDrag->type);
	assert("GargConstraint|Pelvis" == pGargDrag->constraint);
	assert(IsSameFactor(3000, pGargDrag->factors[PhysicBehaviorFactorIdx_BarnacleDragMagnitude]));

	// rigidbody offset factor0 factor1 factor2: each line is one water detection point.
	auto pBuoyancy = FindPhysicBehavior(pConfig.get(), "SimpleBuoyancy|Pelvis");
	assert(pBuoyancy && PhysicBehavior_SimpleBuoyancy == pBuoyancy->type);
	assert("Pelvis" == pBuoyancy->rigidbodyA);
	assert(IsSameFactor(1.5f, pBuoyancy->factors[PhysicBehaviorFactorIdx_SimpleBuoyancyMagnitude]));
	assert(IsSameFactor(0.25f, pBuoyancy->factors[PhysicBehaviorFactorIdx_SimpleBuoyancyLinearDamping]));
	assert(IsSameFactor(0.75f, pBuoyancy->factors[PhysicBehaviorFactorIdx_SimpleBuoyancyAngularDamping]));
	const vec3_t bodyOrigin = { 0, 0, 0 };
	assert(IsNearVector(bodyOrigin, pBuoyancy->origin));
	// Further points on the same rigid body keep their offset and a unique name.
	auto pSecondPoint = FindPhysicBehavior(pConfig.get(), "SimpleBuoyancy|Pelvis|1");
	assert(pSecondPoint && "Pelvis" == pSecondPoint->rigidbodyA);
	const vec3_t secondPointOrigin = { 8, 0, 0 };
	assert(IsNearVector(secondPointOrigin, pSecondPoint->origin));
	auto pSpinePoint = FindPhysicBehavior(pConfig.get(), "SimpleBuoyancy|Spine");
	const vec3_t spinePointOrigin = { 0, 0, 2 };
	assert(pSpinePoint && IsNearVector(spinePointOrigin, pSpinePoint->origin));

	// Without a "Head" rigid body only the third-person camera exists.
	assert(nullptr == FindPhysicBehavior(pConfig.get(), "HeadCamera"));
	auto pThirdPerson = FindPhysicBehavior(pConfig.get(), "PelvisCamera");
	assert(pThirdPerson && PhysicBehavior_ThirdPersonViewCamera == pThirdPerson->type);
	const vec3_t thirdPersonOrigin = { 4, 5, 6 };
	const vec3_t thirdPersonAngles = { 0, 90, 0 };
	assert(IsNearVector(thirdPersonOrigin, pThirdPerson->origin));
	assert(IsNearVector(thirdPersonAngles, pThirdPerson->angles));

	std::set<std::string> names;
	for (const auto& pPhysicBehavior : pConfig->PhysicBehaviorConfigs)
		assert(names.insert(pPhysicBehavior->name).second);
}

static void TestLegacyCameraControl()
{
	auto pConfig = LoadPhysicObjectConfigFromLegacyFileBuffer(
		"[RigidBody]\n"
		"Head 91 12 capsule -4.0 7.0 0.1 6\n"
		"Pelvis 1 8 sphere 2.0 8.0 0.0 10\n"
		"[CameraControl]\n"
		"FirstPerson_AngleOffset 10 0 0\n"
		"FirstPerson_OriginOffset 1 2 3\n"
		"ThirdPerson_OriginOffset 4 5 6\n");
	assert(pConfig);

	// All lines adjust the same two cameras instead of adding new ones.
	assert(2 == pConfig->PhysicBehaviorConfigs.size());
	auto pFirstPerson = FindPhysicBehavior(pConfig.get(), "HeadCamera");
	assert(pFirstPerson && PhysicBehavior_FirstPersonViewCamera == pFirstPerson->type);
	assert("Head" == pFirstPerson->rigidbodyA);
	const vec3_t firstPersonOrigin = { 1, 2, 3 };
	const vec3_t firstPersonAngles = { 10, 0, 0 };
	assert(IsNearVector(firstPersonOrigin, pFirstPerson->origin));
	assert(IsNearVector(firstPersonAngles, pFirstPerson->angles));
	assert(IsSameFactor(1, pFirstPerson->factors[PhysicBehaviorFactorIdx_CameraSyncViewAngles]));

	auto pThirdPerson = FindPhysicBehavior(pConfig.get(), "PelvisCamera");
	assert(pThirdPerson && PhysicBehavior_ThirdPersonViewCamera == pThirdPerson->type);
	const vec3_t thirdPersonOrigin = { 4, 5, 6 };
	assert(IsNearVector(thirdPersonOrigin, pThirdPerson->origin));
	// The third-person camera follows the body position but keeps the player's view angles.
	assert(IsSameFactor(PhysicBehaviorFactorDefaultValue_CameraSyncViewOrigin, pThirdPerson->factors[PhysicBehaviorFactorIdx_CameraSyncViewOrigin]));
	assert(IsSameFactor(0, pThirdPerson->factors[PhysicBehaviorFactorIdx_CameraSyncViewAngles]));
	for (int index = PhysicBehaviorFactorIdx_CameraActivateOnIdle; index <= PhysicBehaviorFactorIdx_CameraNewViewHeightDucking; ++index)
		assert(!std::isnan(pThirdPerson->factors[index]));
}

static void TestLegacyBarnacleConstraints()
{
	auto pConfig = LoadPhysicObjectConfigFromLegacyFileBuffer(
		"[RigidBody]\n"
		"Pelvis 1 8 sphere 2.0 8.0 0.0 10\n"
		"Spine 11 12 capsule 0.5 3.0 6.0 5\n"
		"[Barnacle]\n"
		"Pelvis dof6 0 0 8 1000 24 4\n"
		"Spine slider 1 2 3 500 12 2\n");
	assert(pConfig);

	// rigidbody type offset factor0 factor1 factor2: the tongue pulls the body towards the barnacle.
	auto pDof6 = FindConstraint(pConfig.get(), "BarnacleConstraint|Pelvis");
	assert(pDof6 && PhysicConstraint_Dof6 == pDof6->type);
	assert("@barnacle.Body" == pDof6->rigidbodyA && "Pelvis" == pDof6->rigidbodyB);
	assert(PhysicConstraintFlag_Barnacle == pDof6->flags);
	const vec3_t dof6OriginA = { 0, 0, 24 };
	const vec3_t dof6OriginB = { 0, 0, 8 };
	assert(IsNearVector(dof6OriginA, pDof6->originA) && IsNearVector(dof6OriginB, pDof6->originB));
	assert(pDof6->useLookAtOther && pDof6->useGlobalJointOriginFromOther && pDof6->useRigidBodyDistanceAsLinearLimit);
	assert(IsSameFactor(-4, pDof6->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset]));
	assert(IsSameFactor(-1, pDof6->factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitX]));
	assert(IsSameFactor(0, pDof6->factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitX]));

	auto pSlider = FindConstraint(pConfig.get(), "BarnacleConstraint|Spine");
	assert(pSlider && PhysicConstraint_Slider == pSlider->type);
	const vec3_t sliderOriginB = { 1, 2, 3 };
	assert(IsNearVector(sliderOriginB, pSlider->originB));
	assert(IsSameFactor(-2, pSlider->factors[PhysicConstraintFactorIdx_RigidBodyLinearDistanceOffset]));
	assert(IsSameFactor(-1, pSlider->factors[PhysicConstraintFactorIdx_SliderLowerLinearLimit]));

	for (const char* rigidbody : { "Pelvis", "Spine" })
	{
		auto pDrag = FindPhysicBehavior(pConfig.get(), (std::string("BarnacleDrag|") + rigidbody).c_str());
		assert(pDrag && PhysicBehavior_BarnacleDragOnRigidBody == pDrag->type);
		assert(rigidbody == pDrag->rigidbodyA && PhysicBehaviorFlag_Barnacle == pDrag->flags);
	}
	assert(IsSameFactor(1000, FindPhysicBehavior(pConfig.get(), "BarnacleDrag|Pelvis")->factors[PhysicBehaviorFactorIdx_BarnacleDragMagnitude]));
}

static void TestLegacyParserRejectsInvalidLines()
{
	const char* invalidConfigs[] = {
		"[RigidBody]\nPelvis 1 8 sphere 2 8 0 10\nPelvis 2 8 sphere 2 8 0 10\n",
		"[RigidBody]\nPelvis 1 8 box 2 8 0 10\n",
		"[RigidBody]\nPelvis 1 8 sphere\n",
		"[DeathAnim]\nfifteen 80\n",
		"[Constraint]\nPelvis Spine slider 1 11 0 0 4 0 -4 0 0 0 0\n",
		"[Barnacle]\nPelvis spring 0 0 8 1000 24 4\n",
		"[Gargantua]\nPelvis Body hinge 0 0 0 3000 16 0\n",
	};
	for (const char* invalidConfig : invalidConfigs)
	{
		TestClearConsoleMessage();
		assert(nullptr == LoadPhysicObjectConfigFromLegacyFileBuffer(invalidConfig));
		assert(!TestGetLastConsoleMessage().empty());
	}
}

static void TestLegacyConfigSurvivesEditorSave()
{
	// The editor saves a legacy configuration as *_physics.txt, which then takes precedence.
	auto pLegacy = UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(LoadPhysicObjectConfigFromLegacyFileBuffer(LegacyRagdollConfig));
	auto pSaved = UTIL_ConvertPhysicObjectConfigToRagdollObjectConfig(SaveAndReload(pLegacy.get()));
	assert(pSaved);
	CheckSameObjectHeader(pLegacy.get(), pSaved.get());

	assert(pLegacy->RigidBodyConfigs.size() == pSaved->RigidBodyConfigs.size());
	for (size_t i = 0; i < pLegacy->RigidBodyConfigs.size(); ++i)
		CheckSameRigidBody(pLegacy->RigidBodyConfigs[i].get(), pSaved->RigidBodyConfigs[i].get());

	assert(pLegacy->ConstraintConfigs.size() == pSaved->ConstraintConfigs.size());
	for (size_t i = 0; i < pLegacy->ConstraintConfigs.size(); ++i)
	{
		CheckSameConstraint(pLegacy->ConstraintConfigs[i].get(), pSaved->ConstraintConfigs[i].get());
		for (int index = 0; index < PhysicConstraintFactorIdx_Maximum; ++index)
			assert(IsSameFactor(pLegacy->ConstraintConfigs[i]->factors[index], pSaved->ConstraintConfigs[i]->factors[index]));
	}

	assert(pLegacy->PhysicBehaviorConfigs.size() == pSaved->PhysicBehaviorConfigs.size());
	for (size_t i = 0; i < pLegacy->PhysicBehaviorConfigs.size(); ++i)
	{
		CheckSamePhysicBehavior(pLegacy->PhysicBehaviorConfigs[i].get(), pSaved->PhysicBehaviorConfigs[i].get());
		for (int index = 0; index < PhysicBehaviorFactorIdx_Maximum; ++index)
			assert(IsSameFactor(pLegacy->PhysicBehaviorConfigs[i]->factors[index], pSaved->PhysicBehaviorConfigs[i]->factors[index]));
	}

	assert(pLegacy->AnimControlConfigs.size() == pSaved->AnimControlConfigs.size());
	for (size_t i = 0; i < pLegacy->AnimControlConfigs.size(); ++i)
		CheckSameAnimControl(pLegacy->AnimControlConfigs[i].get(), pSaved->AnimControlConfigs[i].get());
}

static void TestConfigRegistryFollowsConfigLifetime()
{
	auto pShape = std::make_shared<CClientCollisionShapeConfig>();
	auto pRigidBody = std::make_shared<CClientRigidBodyConfig>();
	assert(pShape->configId != pRigidBody->configId);
	ClientPhysicManager()->AddPhysicConfig(pShape->configId, pShape);
	ClientPhysicManager()->AddPhysicConfig(pRigidBody->configId, pRigidBody);

	assert(pRigidBody == UTIL_GetRigidConfigFromConfigId(pRigidBody->configId));
	// Lookups are typed: a collision shape id never resolves as a rigid body.
	assert(nullptr == UTIL_GetRigidConfigFromConfigId(pShape->configId));
	assert(nullptr == UTIL_GetConstraintConfigFromConfigId(pRigidBody->configId));

	// Destroying a configuration unregisters it, so stale ids from the editor resolve to nothing.
	const int shapeId = pShape->configId;
	const int rigidBodyId = pRigidBody->configId;
	pShape.reset();
	pRigidBody.reset();
	assert(ClientPhysicManager()->GetPhysicConfig(shapeId).expired());
	assert(nullptr == UTIL_GetRigidConfigFromConfigId(rigidBodyId));
}

static void TestCloneIsDeepAndMarkedModified()
{
	auto pChild = CreateShape(PhysicShape_Sphere, 3, 0, 0);
	pChild->is_child = true;
	SetVector(pChild->origin, 0, 0, 5);
	auto pCompound = CreateShape(PhysicShape_Compound, 0, 0, 0);
	pCompound->compoundShapes = { pChild };
	auto pRigidBody = CreateRigidBody("Body", pCompound);
	pRigidBody->mass = 7;

	auto pClone = UTIL_CloneRigidBodyConfig(pRigidBody.get());
	CheckSameRigidBody(pRigidBody.get(), pClone.get());
	assert(pClone->configId != pRigidBody->configId);
	assert(pClone->configModified);
	assert(pClone == UTIL_GetRigidConfigFromConfigId(pClone->configId));
	// Editing the clone must not leak into the original's shapes.
	assert(pClone->collisionShape != pRigidBody->collisionShape);
	assert(pClone->collisionShape->compoundShapes[0] != pChild);
	assert(pClone->collisionShape->compoundShapes[0]->configId != pChild->configId);

	auto pConstraint = std::make_shared<CClientConstraintConfig>();
	pConstraint->type = PhysicConstraint_Dof6Spring;
	pConstraint->rotOrder = PhysicRotOrder_ZYX;
	pConstraint->factors[PhysicConstraintFactorIdx_Dof6SpringLinearStiffnessX] = 100;
	auto pConstraintClone = UTIL_CloneConstraintConfig(pConstraint.get());
	CheckSameConstraint(pConstraint.get(), pConstraintClone.get());
	assert(PhysicRotOrder_ZYX == pConstraintClone->rotOrder);
	for (int index = 0; index < PhysicConstraintFactorIdx_Maximum; ++index)
		assert(IsSameFactor(pConstraint->factors[index], pConstraintClone->factors[index]));
}

static void TestModifiedStateGatesSaving()
{
	// bv_save_configs writes only configurations reported as modified.
	auto pChild = CreateShape(PhysicShape_Box, 1, 1, 1);
	auto pCompound = CreateShape(PhysicShape_Compound, 0, 0, 0);
	pCompound->compoundShapes = { pChild };
	auto pConfig = std::make_shared<CClientRagdollObjectConfig>();
	pConfig->RigidBodyConfigs = { CreateRigidBody("Body", pCompound) };
	pConfig->ConstraintConfigs = { std::make_shared<CClientConstraintConfig>() };
	pConfig->PhysicBehaviorConfigs = { std::make_shared<CClientPhysicBehaviorConfig>() };
	pConfig->AnimControlConfigs = { std::make_shared<CClientAnimControlConfig>() };

	CClientBasePhysicConfig* nestedConfigs[] = {
		pConfig.get(), pConfig->RigidBodyConfigs[0].get(), pCompound.get(), pChild.get(),
		pConfig->ConstraintConfigs[0].get(), pConfig->PhysicBehaviorConfigs[0].get(), pConfig->AnimControlConfigs[0].get(),
	};
	for (auto pNested : nestedConfigs)
	{
		UTIL_SetPhysicObjectConfigUnmodified(pConfig.get());
		assert(!UTIL_IsPhysicObjectConfigModified(pConfig.get()));
		pNested->configModified = true;
		assert(UTIL_IsPhysicObjectConfigModified(pConfig.get()));
	}
	UTIL_SetPhysicObjectConfigUnmodified(pConfig.get());
	for (auto pNested : nestedConfigs)
		assert(!pNested->configModified);
}

static void TestEditorReordersAndRemovesComponents()
{
	auto pConfig = std::make_shared<CClientRagdollObjectConfig>();
	auto pA = CreateRigidBody("A", nullptr);
	auto pB = CreateRigidBody("B", nullptr);
	auto pC = CreateRigidBody("C", nullptr);
	pConfig->RigidBodyConfigs = { pA, pB, pC };

	assert(UTIL_ShiftUpRigidBodyIndex(pConfig.get(), pB->configId));
	assert(pB == pConfig->RigidBodyConfigs[0] && pA == pConfig->RigidBodyConfigs[1]);
	assert(!UTIL_ShiftUpRigidBodyIndex(pConfig.get(), pB->configId));
	assert(UTIL_ShiftDownRigidBodyIndex(pConfig.get(), pA->configId));
	assert(pC == pConfig->RigidBodyConfigs[1] && pA == pConfig->RigidBodyConfigs[2]);
	assert(!UTIL_ShiftDownRigidBodyIndex(pConfig.get(), pA->configId));
	assert(1 == UTIL_GetRigidBodyIndex(pConfig.get(), pC->configId));

	pConfig->configModified = false;
	assert(UTIL_RemoveRigidBodyFromPhysicObjectConfig(pConfig.get(), pC->configId));
	assert(pConfig->configModified);
	assert(2 == pConfig->RigidBodyConfigs.size());
	assert(-1 == UTIL_GetRigidBodyIndex(pConfig.get(), pC->configId));
	assert(!UTIL_RemoveRigidBodyFromPhysicObjectConfig(pConfig.get(), pC->configId));
}

static void ThrowSysError(const char* format, ...)
{
	char message[512];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	throw std::runtime_error(message);
}

static void TestSaveByModelIndexValidatesRange()
{
	model_t models[2]{};
	models[0].type = mod_sprite;
	models[1].type = mod_sprite;
	int modelCount = _ARRAYSIZE(models);
	mod_known = models;
	mod_numknown = &modelCount;
	metahook_api_t api{};
	api.SysError = ThrowSysError;
	g_pMetaHookAPI = &api;

	// Preloading sizes the per-model storage; sprites carry no physics configuration.
	ClientPhysicManager()->LoadPhysicObjectConfigs();
	assert(!ClientPhysicManager()->SavePhysicObjectConfigForModelIndex(1));
	assert(!ClientPhysicManager()->SavePhysicObjectConfigForModel(&models[1]));

	for (int modelindex : { -1, EngineGetMaxKnownModel() })
	{
		bool rejected = false;
		try
		{
			ClientPhysicManager()->SavePhysicObjectConfigForModelIndex(modelindex);
		}
		catch (const std::runtime_error&)
		{
			rejected = true;
		}
		assert(rejected);
	}

	g_pMetaHookAPI = nullptr;
	mod_known = nullptr;
	mod_numknown = nullptr;
}

int main()
{
	TestInitPluginRuntime();

	TestTypeNamesRoundTrip();
	TestRagdollConfigRoundTrip();
	TestStaticAndDynamicConfigRoundTrip();
	TestObjectTypeSelectsLoadedGroups();
	TestLoaderAppliesDocumentedDefaults();
	TestLoaderRejectsUnknownObjectTypes();
	TestIntegrityVerificationMatchesModel();
	TestLegacyRagdollFormat();
	TestLegacyCameraControl();
	TestLegacyBarnacleConstraints();
	TestLegacyParserRejectsInvalidLines();
	TestLegacyConfigSurvivesEditorSave();
	TestConfigRegistryFollowsConfigLifetime();
	TestCloneIsDeepAndMarkedModified();
	TestModifiedStateGatesSaving();
	TestEditorReordersAndRemovesComponents();
	TestSaveByModelIndexValidatesRange();

	TestShutdownPluginRuntime();
	puts("Physics configuration tests passed.");
	return 0;
}
