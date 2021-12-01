#pragma once

#include <Magnum/SceneGraph/Scene.h>

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class RigidBody: public Object3D {
    public:
        RigidBody(Object3D* parent, Float mass, btCollisionShape* bShape, btDynamicsWorld& bWorld): Object3D{parent}, _bWorld(bWorld) {
            /* Calculate inertia so the object reacts as it should with
               rotation and everything */
            btVector3 bInertia(0.0f, 0.0f, 0.0f);
            if(!Math::TypeTraits<Float>::equals(mass, 0.0f))
                bShape->calculateLocalInertia(mass, bInertia);

            /* Bullet rigid body setup */
            auto* motionState = new BulletIntegration::MotionState{*this};
            _bRigidBody.emplace(btRigidBody::btRigidBodyConstructionInfo{
                mass, &motionState->btMotionState(), bShape, bInertia});
            _bRigidBody->forceActivationState(DISABLE_DEACTIVATION);
            bWorld.addRigidBody(_bRigidBody.get());
        }

        ~RigidBody() {
            _bWorld.removeRigidBody(_bRigidBody.get());
        }

        btRigidBody& rigidBody() { return *_bRigidBody; }

        /* needed after changing the pose from Magnum side */
        void syncPose() {
            _bRigidBody->setWorldTransform(btTransform(transformationMatrix()));
        }

    private:
        btDynamicsWorld& _bWorld;
        Containers::Pointer<btRigidBody> _bRigidBody;
};