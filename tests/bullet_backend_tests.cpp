// Exercise the translation from physics configurations and GoldSrc transforms into Bullet.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

#include <cvardef.h>

#include "BulletPhysicManager.h"
#include "mathlib2.h"
#include "test_support.h"

extern cvar_t* bv_simrate;

static bool IsNearVector3(const btVector3& expected, const btVector3& actual, float tolerance = 1e-4f)
{
    return IsNear(expected.x(), actual.x(), tolerance) && IsNear(expected.y(), actual.y(), tolerance) && IsNear(expected.z(), actual.z(), tolerance);
}

static bool IsNearBasis(const btMatrix3x3& expected, const btMatrix3x3& actual, float tolerance = 1e-4f)
{
    return IsNearVector3(expected[0], actual[0], tolerance) && IsNearVector3(expected[1], actual[1], tolerance) && IsNearVector3(expected[2], actual[2], tolerance);
}

static bool IsNearTransform(const btTransform& expected, const btTransform& actual, float tolerance = 1e-4f)
{
    return IsNearBasis(expected.getBasis(), actual.getBasis(), tolerance) && IsNearVector3(expected.getOrigin(), actual.getOrigin(), tolerance);
}

static btTransform MakeTransform(const btVector3& angles, const btVector3& origin)
{
    btTransform transform;
    transform.setIdentity();
    EulerMatrix(angles, transform.getBasis());
    transform.setOrigin(origin);
    return transform;
}

// Bullet is built without RTTI, so downcasts are checked against its own type ids.
template <typename T>
static T* AsShape(btCollisionShape* pShape, int shapeType)
{
    assert(pShape && shapeType == pShape->getShapeType());
    return static_cast<T*>(pShape);
}

template <typename T>
static T* AsConstraint(btTypedConstraint* pConstraint, btTypedConstraintType constraintType)
{
    assert(pConstraint && constraintType == pConstraint->getConstraintType());
    return static_cast<T*>(pConstraint);
}

static void TestEulerAnglesRoundTrip()
{
    // Configuration angles are stored as Euler angles and rebuilt with EulerMatrix.
    const btVector3 anglesList[] = {
        btVector3(0, 0, 0),
        btVector3(30, 45, 60),
        btVector3(-20, 170, -95),
        btVector3(-60, -120, 135),
        btVector3(90, 30, 10),
        btVector3(-90, 30, 10),
    };
    for (const auto& angles : anglesList)
    {
        btMatrix3x3 basis;
        EulerMatrix(angles, basis);
        assert(IsNear(1, basis.determinant()));
        assert(IsNearBasis(btMatrix3x3::getIdentity(), basis * basis.transpose()));

        btVector3 recovered;
        MatrixEuler(basis, recovered);
        btMatrix3x3 rebuilt;
        EulerMatrix(recovered, rebuilt);
        assert(IsNearBasis(basis, rebuilt));
    }
}

static void TestBoneMatrixConversion()
{
    // Studio bone matrices are GoldSrc 3x4 matrices; rigid bodies exchange them as btTransform.
    const vec3_t angles = {15, -40, 70};
    float        bone[3][4];
    AngleMatrix(angles, bone);
    bone[0][3] = 12;
    bone[1][3] = -34;
    bone[2][3] = 56;

    btTransform transform;
    Matrix3x4ToTransform(bone, transform);

    const vec3_t point = {3, -7, 11};
    vec3_t       expected;
    VectorTransform(point, bone, expected);
    const auto actual = transform * btVector3(point[0], point[1], point[2]);
    assert(IsNearVector3(btVector3(expected[0], expected[1], expected[2]), actual));

    float roundTrip[3][4];
    TransformToMatrix3x4(transform, roundTrip);
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 4; ++column)
            assert(IsNear(bone[row][column], roundTrip[row][column]));
}

static void TestBoneMotionStateKeepsBodyOffset()
{
    const auto             bone   = MakeTransform(btVector3(10, 20, 30), btVector3(1, 2, 3));
    const auto             offset = MakeTransform(btVector3(0, 90, 0), btVector3(0, 0, 4));
    CBulletBoneMotionState motionState(nullptr, bone, offset);

    btTransform world;
    motionState.getWorldTransform(world);
    assert(IsNearTransform(bone * offset, world));

    // When Bullet moves the body, the bone follows with the same body-to-bone offset.
    const auto simulated = MakeTransform(btVector3(-45, 0, 15), btVector3(100, -50, 25));
    motionState.setWorldTransform(simulated);
    motionState.getWorldTransform(world);
    assert(IsNearTransform(simulated, world));
    assert(IsNearTransform(offset, motionState.m_bonematrix.inverse() * simulated));
}

static void TestLookAtRotation()
{
    const btVector3 directions[][2] = {
        {btVector3(1, 0, 0), btVector3(0, 1, 0)},
        {btVector3(0, 2, 0), btVector3(0, 0, -3)},
        {btVector3(1, 1, 0), btVector3(-1, 2, 5)},
        {btVector3(1, 0, 0), btVector3(-4, 0, 0)},
        // Opposite directions along the up axis need a different rotation axis.
        {btVector3(0, 0, 1), btVector3(0, 0, -3)},
    };
    for (const auto& direction : directions)
    {
        const auto rotation = FromToRotaion(direction[0], direction[1]);
        assert(IsNearVector3(direction[1].normalized(), quatRotate(rotation, direction[0].normalized())));
    }

    // Constraints with useLookAtOther aim the body's forward axis at the other body.
    const auto      transform = MakeTransform(btVector3(0, 90, 0), btVector3(10, 0, 0));
    const btVector3 forward(1, 0, 0);
    const btVector3 target(10, 50, 20);
    const auto      aimed = MatrixLookAt(transform, target, forward);
    assert(IsNearVector3(transform.getOrigin(), aimed.getOrigin()));
    assert(IsNearVector3((target - transform.getOrigin()).normalized(), aimed.getBasis() * forward));
}

static std::shared_ptr<CClientCollisionShapeConfig> CreateShape(int type, float x, float y, float z, int direction = PhysicShapeDirection_Y)
{
    auto pShape       = std::make_shared<CClientCollisionShapeConfig>();
    pShape->type      = type;
    pShape->direction = direction;
    pShape->size[0]   = x;
    pShape->size[1]   = y;
    pShape->size[2]   = z;
    return pShape;
}

static btCollisionShape* CreateBulletShape(const std::shared_ptr<CClientCollisionShapeConfig>& pShape)
{
    CClientRigidBodyConfig rigidBody;
    rigidBody.collisionShape = pShape;
    return BulletCreateCollisionShape(&rigidBody);
}

static void DeleteBulletShape(btCollisionShape* pShape)
{
    if (pShape->isCompound())
    {
        auto pCompound = static_cast<btCompoundShape*>(pShape);
        for (int i = pCompound->getNumChildShapes() - 1; i >= 0; --i)
            DeleteBulletShape(pCompound->getChildShape(i));
    }
    OnBeforeDeleteBulletCollisionShape(pShape);
    delete pShape;
}

static void TestCollisionShapesFollowConfig()
{
    // Sphere size[0] is the radius.
    auto pSphere = AsShape<btSphereShape>(CreateBulletShape(CreateShape(PhysicShape_Sphere, 4, 0, 0)), SPHERE_SHAPE_PROXYTYPE);
    assert(IsNear(4 * G2BScale, pSphere->getRadius()));
    DeleteBulletShape(pSphere);

    // Box size holds half extents.
    auto pBox = AsShape<btBoxShape>(CreateBulletShape(CreateShape(PhysicShape_Box, 2, 3, 4)), BOX_SHAPE_PROXYTYPE);
    assert(IsNearVector3(btVector3(2, 3, 4) * G2BScale, pBox->getHalfExtentsWithMargin()));
    DeleteBulletShape(pBox);

    // Capsule size[0] is the radius and size[1] the cylinder height along "direction".
    for (int direction = PhysicShapeDirection_X; direction < PhysicShapeDirection_Maximum; ++direction)
    {
        auto pCapsule = AsShape<btCapsuleShape>(CreateBulletShape(CreateShape(PhysicShape_Capsule, 3, 10, 0, direction)), CAPSULE_SHAPE_PROXYTYPE);
        assert(direction == pCapsule->getUpAxis());
        assert(IsNear(3 * G2BScale, pCapsule->getRadius()));
        assert(IsNear(5 * G2BScale, pCapsule->getHalfHeight()));
        DeleteBulletShape(pCapsule);

        auto pCylinder = AsShape<btCylinderShape>(CreateBulletShape(CreateShape(PhysicShape_Cylinder, 2, 6, 4, direction)), CYLINDER_SHAPE_PROXYTYPE);
        assert(direction == pCylinder->getUpAxis());
        assert(IsNearVector3(btVector3(2, 6, 4) * G2BScale, pCylinder->getHalfExtentsWithMargin()));
        DeleteBulletShape(pCylinder);
    }

    // Compound children are placed with their own origin and Euler angles.
    auto pSphereChild               = CreateShape(PhysicShape_Sphere, 1, 0, 0);
    pSphereChild->origin[2]         = 5;
    auto pBoxChild                  = CreateShape(PhysicShape_Box, 1, 2, 3);
    pBoxChild->origin[0]            = 1;
    pBoxChild->origin[1]            = 2;
    pBoxChild->origin[2]            = 3;
    pBoxChild->angles[0]            = 10;
    pBoxChild->angles[1]            = 20;
    pBoxChild->angles[2]            = 30;
    auto pCompoundConfig            = CreateShape(PhysicShape_Compound, 0, 0, 0);
    pCompoundConfig->compoundShapes = {pSphereChild, pBoxChild};
    auto pCompound                  = AsShape<btCompoundShape>(CreateBulletShape(pCompoundConfig), COMPOUND_SHAPE_PROXYTYPE);
    assert(2 == pCompound->getNumChildShapes());
    assert(IsNearTransform(MakeTransform(btVector3(0, 0, 0), btVector3(0, 0, 5) * G2BScale), pCompound->getChildTransform(0)));
    assert(IsNearTransform(MakeTransform(btVector3(10, 20, 30), btVector3(1, 2, 3) * G2BScale), pCompound->getChildTransform(1)));
    assert(BOX_SHAPE_PROXYTYPE == pCompound->getChildShape(1)->getShapeType());
    DeleteBulletShape(pCompound);

    // Configurations that cannot produce a collider are rejected instead of yielding empty bodies.
    assert(nullptr == CreateBulletShape(CreateShape(PhysicShape_None, 1, 1, 1)));
    auto pEmptyCompound            = CreateShape(PhysicShape_Compound, 0, 0, 0);
    pEmptyCompound->compoundShapes = {CreateShape(PhysicShape_None, 1, 1, 1)};
    assert(nullptr == CreateBulletShape(pEmptyCompound));
    TestClearConsoleMessage();
    assert(nullptr == CreateBulletShape(CreateShape(PhysicShape_TriangleMesh, 0, 0, 0)));
    assert(!TestGetLastConsoleMessage().empty());

    // A missing mesh is reported and cached as failed instead of throwing into the engine.
    const char* missingMesh    = "models/bulletphysics_missing_mesh.obj";
    auto        pMissingMesh   = CreateShape(PhysicShape_TriangleMesh, 0, 0, 0);
    pMissingMesh->resourcePath = missingMesh;
    TestClearConsoleMessage();
    assert(nullptr == CreateBulletShape(pMissingMesh));
    assert(!TestGetLastConsoleMessage().empty());
    auto pFailedIndexArray = ClientPhysicManager()->LoadIndexArrayFromResource(missingMesh);
    assert(pFailedIndexArray && (PhysicIndexArrayFlag_LoadFailed & pFailedIndexArray->flags));
    TestClearConsoleMessage();
    assert(nullptr == CreateBulletShape(nullptr));
    assert(!TestGetLastConsoleMessage().empty());
}

class CTestConstraintBodies
{
public:
    CTestConstraintBodies() :
        m_shape(1),
        m_bodyA(btRigidBody::btRigidBodyConstructionInfo(1, nullptr, &m_shape, btVector3(1, 1, 1))),
        m_bodyB(btRigidBody::btRigidBodyConstructionInfo(1, nullptr, &m_shape, btVector3(1, 1, 1)))
    {
        m_context.pRigidBodyA       = &m_bodyA;
        m_context.pRigidBodyB       = &m_bodyB;
        m_context.rigidBodyDistance = 10;
        m_frameA                    = MakeTransform(btVector3(0, 0, 0), btVector3(0, 0, 1));
        m_frameB                    = MakeTransform(btVector3(0, 0, 0), btVector3(0, 0, -1));
    }

    btTypedConstraint* Create(const CClientConstraintConfig& config)
    {
        return BulletCreateConstraintFromLocalJointTransform(&config, m_context, m_frameA, m_frameB);
    }

    btSphereShape                    m_shape;
    btRigidBody                      m_bodyA;
    btRigidBody                      m_bodyB;
    CBulletConstraintCreationContext m_context;
    btTransform                      m_frameA;
    btTransform                      m_frameB;
};

static void TestConstraintsFollowConfig()
{
    CTestConstraintBodies bodies;

    // Angular limits are stored as fractions of PI; missing factors fall back to Bullet defaults.
    CClientConstraintConfig coneTwistConfig;
    coneTwistConfig.type                                                        = PhysicConstraint_ConeTwist;
    coneTwistConfig.factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit1] = 0.25f;
    coneTwistConfig.factors[PhysicConstraintFactorIdx_ConeTwistSwingSpanLimit2] = 0.5f;
    coneTwistConfig.factors[PhysicConstraintFactorIdx_ConeTwistTwistSpanLimit]  = 0.125f;
    auto pConeTwist                                                             = AsConstraint<btConeTwistConstraint>(bodies.Create(coneTwistConfig), CONETWIST_CONSTRAINT_TYPE);
    assert(IsNear(0.25f * SIMD_PI, pConeTwist->getSwingSpan1()));
    assert(IsNear(0.5f * SIMD_PI, pConeTwist->getSwingSpan2()));
    assert(IsNear(0.125f * SIMD_PI, pConeTwist->getTwistSpan()));
    assert(IsNear(BULLET_DEFAULT_SOFTNESS, pConeTwist->getLimitSoftness()));
    assert(IsNear(BULLET_DEFAULT_BIAS_FACTOR, pConeTwist->getBiasFactor()));
    assert(IsNear(BULLET_DEFAULT_RELAXTION_FACTOR, pConeTwist->getRelaxationFactor()));
    delete pConeTwist;

    CClientConstraintConfig hingeConfig;
    hingeConfig.type                                              = PhysicConstraint_Hinge;
    hingeConfig.factors[PhysicConstraintFactorIdx_HingeLowLimit]  = -0.5f;
    hingeConfig.factors[PhysicConstraintFactorIdx_HingeHighLimit] = 0.25f;
    hingeConfig.factors[PhysicConstraintFactorIdx_AngularStopERP] = 0.8f;
    hingeConfig.factors[PhysicConstraintFactorIdx_AngularStopCFM] = 0.02f;
    auto pHinge                                                   = AsConstraint<btHingeConstraint>(bodies.Create(hingeConfig), HINGE_CONSTRAINT_TYPE);
    assert(IsNear(-0.5f * SIMD_PI, pHinge->getLowerLimit()));
    assert(IsNear(0.25f * SIMD_PI, pHinge->getUpperLimit()));
    assert(IsNear(BULLET_DEFAULT_SOFTNESS, pHinge->getLimitSoftness()));
    assert(IsNear(BULLET_DEFAULT_BIAS_FACTOR, pHinge->getLimitBiasFactor()));
    assert(IsNear(BULLET_DEFAULT_RELAXTION_FACTOR, pHinge->getLimitRelaxationFactor()));
    assert(IsNear(0.8f, pHinge->getParam(BT_CONSTRAINT_STOP_ERP, 5)));
    assert(IsNear(0.02f, pHinge->getParam(BT_CONSTRAINT_STOP_CFM, 5)));
    delete pHinge;

    CClientConstraintConfig pointConfig;
    pointConfig.type = PhysicConstraint_Point;
    auto pPoint      = AsConstraint<btPoint2PointConstraint>(bodies.Create(pointConfig), POINT2POINT_CONSTRAINT_TYPE);
    assert(IsNearVector3(bodies.m_frameA.getOrigin(), pPoint->getPivotInA()));
    assert(IsNearVector3(bodies.m_frameB.getOrigin(), pPoint->getPivotInB()));
    delete pPoint;

    // Barnacle constraints scale linear limits by the current distance between the bodies.
    CClientConstraintConfig sliderConfig;
    sliderConfig.type                                                       = PhysicConstraint_Slider;
    sliderConfig.useRigidBodyDistanceAsLinearLimit                          = true;
    sliderConfig.useLinearReferenceFrameA                                   = false;
    sliderConfig.factors[PhysicConstraintFactorIdx_SliderLowerLinearLimit]  = -0.25f;
    sliderConfig.factors[PhysicConstraintFactorIdx_SliderUpperLinearLimit]  = 0.5f;
    sliderConfig.factors[PhysicConstraintFactorIdx_SliderLowerAngularLimit] = -0.5f;
    sliderConfig.factors[PhysicConstraintFactorIdx_SliderUpperAngularLimit] = 0.5f;
    auto pSlider                                                            = AsConstraint<btSliderConstraint>(bodies.Create(sliderConfig), SLIDER_CONSTRAINT_TYPE);
    assert(!pSlider->getUseLinearReferenceFrameA());
    assert(IsNear(-0.25f * bodies.m_context.rigidBodyDistance, pSlider->getLowerLinLimit()));
    assert(IsNear(0.5f * bodies.m_context.rigidBodyDistance, pSlider->getUpperLinLimit()));
    assert(IsNear(-0.5f * SIMD_PI, pSlider->getLowerAngLimit()));
    assert(IsNear(0.5f * SIMD_PI, pSlider->getUpperAngLimit()));
    delete pSlider;

    CClientConstraintConfig dof6Config;
    dof6Config.type                                                      = PhysicConstraint_Dof6;
    dof6Config.factors[PhysicConstraintFactorIdx_Dof6LowerLinearLimitX]  = -1;
    dof6Config.factors[PhysicConstraintFactorIdx_Dof6UpperLinearLimitX]  = 2;
    dof6Config.factors[PhysicConstraintFactorIdx_Dof6LowerAngularLimitX] = -0.25f;
    dof6Config.factors[PhysicConstraintFactorIdx_Dof6UpperAngularLimitX] = 0.25f;
    dof6Config.factors[PhysicConstraintFactorIdx_LinearStopERP]          = 0.7f;
    dof6Config.factors[PhysicConstraintFactorIdx_AngularStopCFM]         = 0.03f;
    auto      pDof6                                                      = AsConstraint<btGeneric6DofConstraint>(bodies.Create(dof6Config), D6_CONSTRAINT_TYPE);
    btVector3 lower;
    btVector3 upper;
    pDof6->getLinearLowerLimit(lower);
    pDof6->getLinearUpperLimit(upper);
    assert(IsNearVector3(btVector3(-1, 0, 0), lower));
    assert(IsNearVector3(btVector3(2, 0, 0), upper));
    pDof6->getAngularLowerLimit(lower);
    pDof6->getAngularUpperLimit(upper);
    // Unspecified angular axes default to a full turn either way.
    assert(IsNearVector3(btVector3(-0.25f, -1, -1) * SIMD_PI, lower));
    assert(IsNearVector3(btVector3(0.25f, 1, 1) * SIMD_PI, upper));
    for (int axis = 0; axis < 3; ++axis)
    {
        assert(IsNear(0.7f, pDof6->getParam(BT_CONSTRAINT_STOP_ERP, axis)));
        assert(IsNear(0.03f, pDof6->getParam(BT_CONSTRAINT_STOP_CFM, axis + 3)));
    }
    delete pDof6;

    CClientConstraintConfig springConfig;
    springConfig.type                                                              = PhysicConstraint_Dof6Spring;
    springConfig.rotOrder                                                          = PhysicRotOrder_ZYX;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringEnableLinearSpringX]  = 1;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringLinearStiffnessX]     = 100;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringLinearDampingX]       = 0.5f;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringEnableAngularSpringY] = 1;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringAngularStiffnessY]    = 20;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringAngularDampingY]      = 0.125f;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringEnableAngularSpringZ] = 1;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringAngularStiffnessZ]    = 30;
    springConfig.factors[PhysicConstraintFactorIdx_Dof6SpringAngularDampingZ]      = 0.25f;
    auto pSpring                                                                   = AsConstraint<btGeneric6DofSpring2Constraint>(bodies.Create(springConfig), D6_SPRING_2_CONSTRAINT_TYPE);
    assert(RO_ZYX == pSpring->getRotationOrder());
    assert(pSpring->getTranslationalLimitMotor()->m_enableSpring[0]);
    assert(IsNear(100, pSpring->getTranslationalLimitMotor()->m_springStiffness[0]));
    assert(IsNear(0.5f, pSpring->getTranslationalLimitMotor()->m_springDamping[0]));
    assert(!pSpring->getTranslationalLimitMotor()->m_enableSpring[1]);
    assert(pSpring->getRotationalLimitMotor(2)->m_enableSpring);
    assert(IsNear(30, pSpring->getRotationalLimitMotor(2)->m_springStiffness));
    assert(IsNear(0.25f, pSpring->getRotationalLimitMotor(2)->m_springDamping));
    // Each axis keeps its own spring parameters.
    assert(pSpring->getRotationalLimitMotor(1)->m_enableSpring);
    assert(IsNear(20, pSpring->getRotationalLimitMotor(1)->m_springStiffness));
    assert(IsNear(0.125f, pSpring->getRotationalLimitMotor(1)->m_springDamping));
    assert(!pSpring->getRotationalLimitMotor(0)->m_enableSpring);
    assert(IsNear(0, pSpring->getRotationalLimitMotor(0)->m_springStiffness));
    delete pSpring;

    CClientConstraintConfig fixedConfig;
    fixedConfig.type = PhysicConstraint_Fixed;
    // A fixed joint locks every axis.
    auto pFixed = AsConstraint<btGeneric6DofSpring2Constraint>(bodies.Create(fixedConfig), D6_SPRING_2_CONSTRAINT_TYPE);
    pFixed->getLinearLowerLimit(lower);
    pFixed->getLinearUpperLimit(upper);
    assert(IsNearVector3(btVector3(0, 0, 0), lower) && IsNearVector3(btVector3(0, 0, 0), upper));
    pFixed->getAngularLowerLimit(lower);
    pFixed->getAngularUpperLimit(upper);
    assert(IsNearVector3(btVector3(0, 0, 0), lower) && IsNearVector3(btVector3(0, 0, 0), upper));
    delete pFixed;

    CClientConstraintConfig noneConfig;
    assert(nullptr == bodies.Create(noneConfig));
}

static void TestLegacyJointMigrationKeepsJointPlacement()
{
    // Legacy constraints are converted to originA/anglesA on first build, which a later save keeps.
    CTestConstraintBodies bodies;
    const auto            worldA    = MakeTransform(btVector3(0, 90, 0), btVector3(10, 0, 0));
    const auto            worldB    = MakeTransform(btVector3(20, -30, 45), btVector3(0, 20, 5));
    bodies.m_context.invWorldTransA = worldA.inverse();
    bodies.m_context.invWorldTransB = worldB.inverse();
    const auto globalJoint          = MakeTransform(btVector3(5, 25, -40), btVector3(3, 4, 5));

    CClientConstraintConfig config;
    config.type           = PhysicConstraint_ConeTwist;
    config.isLegacyConfig = true;
    auto pConstraint      = BulletCreateConstraintFromGlobalJointTransform(&config, bodies.m_context, globalJoint);
    assert(pConstraint);
    assert(!config.isLegacyConfig);

    // Rebuild the frames the way BulletRagdollObject does for non-legacy constraints.
    const auto localA = MakeTransform(GetVector3FromVec3(config.anglesA), GetVector3FromVec3(config.originA) * G2BScale);
    const auto localB = MakeTransform(GetVector3FromVec3(config.anglesB), GetVector3FromVec3(config.originB) * G2BScale);
    assert(IsNearTransform(globalJoint, worldA * localA, 1e-3f));
    assert(IsNearTransform(globalJoint, worldB * localB, 1e-3f));
    delete pConstraint;
}

static std::string s_LastCvarName;
static float       s_LastCvarValue;

static void TestCvarSetValue(const char* name, float value)
{
    s_LastCvarName    = name;
    s_LastCvarValue   = value;
    bv_simrate->value = value;
}

static void TestSimulationRateIsClamped()
{
    cvar_t simrate{};
    bv_simrate              = &simrate;
    gEngfuncs.Cvar_SetValue = TestCvarSetValue;

    ClientPhysicManager()->Init();
    ClientPhysicManager()->SetGravity(800);

    // bv_simrate is kept within 32..128 ticks per second before stepping.
    const struct
    {
        float requested;
        float expected;
    } cases[] = {{10, 32}, {500, 128}, {64, 64}};
    for (const auto& test : cases)
    {
        simrate.value = test.requested;
        s_LastCvarName.clear();
        ClientPhysicManager()->StepSimulation(1.0 / 60.0);
        assert(IsNear(test.expected, simrate.value));
        assert((test.requested == test.expected) == s_LastCvarName.empty());
        if (!s_LastCvarName.empty())
            assert("bv_simrate" == s_LastCvarName && IsNear(test.expected, s_LastCvarValue));
    }

    ClientPhysicManager()->Shutdown();
    bv_simrate = nullptr;
}

static void TestPhysicObjectIdKeepsModelIdentity()
{
    // Entity slots are reused; the packed id also records the model to reject stale objects.
    const int  entindex       = 1234;
    const int  modelindex     = 16383;
    const auto physicObjectId = PACK_PHYSIC_OBJECT_ID(entindex, modelindex);
    assert(entindex == UNPACK_PHYSIC_OBJECT_ID_TO_ENTINDEX(physicObjectId));
    assert(modelindex == UNPACK_PHYSIC_OBJECT_ID_TO_MODELINDEX(physicObjectId));
    assert(physicObjectId != PACK_PHYSIC_OBJECT_ID(entindex, modelindex - 1));
}

int main()
{
    TestInitPluginRuntime();

    TestEulerAnglesRoundTrip();
    TestBoneMatrixConversion();
    TestBoneMotionStateKeepsBodyOffset();
    TestLookAtRotation();
    TestCollisionShapesFollowConfig();
    TestConstraintsFollowConfig();
    TestLegacyJointMigrationKeepsJointPlacement();
    TestSimulationRateIsClamped();
    TestPhysicObjectIdKeepsModelIdentity();

    TestShutdownPluginRuntime();
    puts("Bullet backend tests passed.");
    return 0;
}
