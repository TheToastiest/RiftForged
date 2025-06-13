// File: Engine/PhysicsEngine/src/PhysicsActorManagement.cpp
// Copyright (c) 2025-2028 RiftForged Game Development Team

#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/Utilities/Logger/Logger.h>

namespace RiftForged {
    namespace Physics {

        void PhysicsEngine::RegisterPlayerController(uint64_t player_id, physx::PxController* controller) {
            if (!controller) {
                RF_PHYSICS_ERROR("RegisterPlayerController: Attempted to register null PxController for player ID {}.", player_id);
                return;
            }
            std::lock_guard<std::mutex> lock(m_playerControllersMutex);
            m_playerControllers[player_id] = controller;
            RF_PHYSICS_INFO("Registered PxController for player ID {}.", player_id);
        }

        void PhysicsEngine::UnregisterPlayerController(uint64_t player_id) {
            physx::PxController* controller_to_release = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_playerControllersMutex);
                auto it = m_playerControllers.find(player_id);
                if (it != m_playerControllers.end()) {
                    controller_to_release = it->second;
                    m_playerControllers.erase(it);
                }
            }
            if (controller_to_release) {
                std::lock_guard<std::mutex> lock(m_physicsMutex);
                controller_to_release->release();
                RF_PHYSICS_INFO("Unregistered and released PxController for player ID {}.", player_id);
            }
        }

        physx::PxController* PhysicsEngine::GetPlayerController(uint64_t player_id) const {
            std::lock_guard<std::mutex> lock(m_playerControllersMutex);
            auto it = m_playerControllers.find(player_id);
            return (it != m_playerControllers.end()) ? it->second : nullptr;
        }

        uint32_t PhysicsEngine::MoveCharacterController(physx::PxController* controller, const SharedVec3& world_space_displacement, float delta_time_sec, const std::vector<physx::PxController*>& other_controllers_to_ignore) {
            (void)other_controllers_to_ignore;
            if (!controller) return 0;
            if (delta_time_sec <= 0.0f) return 0;

            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!controller->getScene()) return 0;

            physx::PxControllerFilters filters;
            // TODO: Implement logic to ignore other controllers if necessary
            return controller->move(ToPxVec3(world_space_displacement), 0.001f, delta_time_sec, filters);
        }

        void PhysicsEngine::SetCharacterControllerPose(physx::PxController* controller, const SharedVec3& world_position) {
            if (!controller) return;
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (controller->getScene()) {
                controller->setPosition(physx::PxExtendedVec3(world_position.x, world_position.y, world_position.z));
            }
        }

        bool PhysicsEngine::SetCharacterControllerOrientation(uint64_t player_id, const SharedQuaternion& orientation) {
            physx::PxController* controller = GetPlayerController(player_id);
            if (!controller) return false;
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            physx::PxRigidActor* actor = controller->getActor();
            if (!actor || !actor->getScene()) return false;
            physx::PxTransform currentPose = actor->getGlobalPose();
            currentPose.q = ToPxQuat(orientation);
            actor->setGlobalPose(currentPose);
            return true;
        }

        SharedVec3 PhysicsEngine::GetCharacterControllerPosition(physx::PxController* controller) const {
            if (!controller) return SharedVec3(0.0f);
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            physx::PxExtendedVec3 pos = controller->getPosition();
            return FromPxVec3(physx::PxVec3(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z)));
        }

        void PhysicsEngine::SetActorUserData(physx::PxActor* actor, void* userData) {
            if (actor) {
                actor->userData = userData;
            }
        }

        void PhysicsEngine::RegisterRigidActor(uint64_t entity_id, physx::PxRigidActor* actor) {
            if (!actor) return;
            std::lock_guard<std::mutex> lock(m_entityActorsMutex);
            m_entityActors[entity_id] = actor;
        }

        void PhysicsEngine::UnregisterRigidActor(uint64_t entity_id) {
            physx::PxRigidActor* actor_to_release = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_entityActorsMutex);
                auto it = m_entityActors.find(entity_id);
                if (it != m_entityActors.end()) {
                    actor_to_release = it->second;
                    m_entityActors.erase(it);
                }
            }
            if (actor_to_release) {
                std::lock_guard<std::mutex> lock(m_physicsMutex);
                if (m_scene && actor_to_release->getScene()) {
                    m_scene->removeActor(*actor_to_release);
                }
                actor_to_release->release();
            }
        }

        physx::PxRigidActor* PhysicsEngine::GetRigidActor(uint64_t entity_id) const {
            std::lock_guard<std::mutex> lock(m_entityActorsMutex);
            auto it = m_entityActors.find(entity_id);
            return (it != m_entityActors.end()) ? it->second : nullptr;
        }

        void PhysicsEngine::ApplyForceToActor(physx::PxRigidBody* actor, const SharedVec3& force, physx::PxForceMode::Enum mode, bool wakeup) {
            if (!actor) return;
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (actor->getScene()) {
                actor->addForce(ToPxVec3(force), mode, wakeup);
            }
        }

        void PhysicsEngine::ApplyForceToActorById(uint64_t entity_id, const SharedVec3& force, physx::PxForceMode::Enum mode, bool wakeup) {
            physx::PxRigidActor* actor_base = GetRigidActor(entity_id);
            if (actor_base && actor_base->is<physx::PxRigidBody>()) {
                ApplyForceToActor(static_cast<physx::PxRigidBody*>(actor_base), force, mode, wakeup);
            }
        }

        // Stubs for Advanced Features
        void PhysicsEngine::CreateRadialForceField(uint64_t, const SharedVec3&, float, float, float, bool, float) { RF_PHYSICS_WARN("CreateRadialForceField: Not yet implemented."); }
        void PhysicsEngine::ApplyLocalizedGravity(const SharedVec3&, float, float, float, const SharedVec3&) { RF_PHYSICS_WARN("ApplyLocalizedGravity: Not yet implemented."); }
        bool PhysicsEngine::DeformTerrainRegion(const SharedVec3&, float, float, int) { RF_PHYSICS_WARN("DeformTerrainRegion: Not yet implemented."); return false; }

    } // namespace Physics
} // namespace RiftForged