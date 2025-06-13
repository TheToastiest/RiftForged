// File: Engine/PhysicsEngine/src/PhysicsSceneQuery.cpp
// Copyright (c) 2025-2028 RiftForged Game Development Team

#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>
#include <RiftForged/Utilities/Logger/Logger.h>

// Note: Most necessary PhysX types like PxQueryFilterData, PxSweepBuffer, etc.
// are included via PhysicsEngine.h -> PhysicsTypes.h -> PxPhysicsAPI.h

namespace RiftForged {
    namespace Physics {

        // This local filter callback is an implementation detail of the sweep function below.
        // Keeping it in the .cpp file hides it from the rest of the engine.
        struct RiftStepSweepQueryFilterCallback : public physx::PxQueryFilterCallback {
            physx::PxRigidActor* m_actorToIgnore;

            RiftStepSweepQueryFilterCallback(physx::PxRigidActor* actorToIgnore = nullptr)
                : m_actorToIgnore(actorToIgnore) {
            }

            virtual physx::PxQueryHitType::Enum preFilter(
                const physx::PxFilterData& shapeFilterData,
                const physx::PxShape* shape,
                const physx::PxRigidActor* hitActor,
                physx::PxHitFlags& queryFlags) override
            {
                (void)shape;
                (void)queryFlags;
                if (hitActor == m_actorToIgnore) {
                    return physx::PxQueryHitType::eNONE; // Ignore self
                }

                // Example filtering logic based on object type
                EPhysicsObjectType hitObjectType = static_cast<EPhysicsObjectType>(shapeFilterData.word0);
                if (hitObjectType == EPhysicsObjectType::WALL ||
                    hitObjectType == EPhysicsObjectType::IMPASSABLE_ROCK ||
                    hitObjectType == EPhysicsObjectType::STATIC_IMPASSABLE) {
                    return physx::PxQueryHitType::eBLOCK;
                }
                // Allow passing through less significant objects
                if (hitObjectType == EPhysicsObjectType::PLAYER_CHARACTER ||
                    hitObjectType == EPhysicsObjectType::SMALL_ENEMY) {
                    return physx::PxQueryHitType::eTOUCH;
                }

                return physx::PxQueryHitType::eBLOCK; // Default to block
            }

            virtual physx::PxQueryHitType::Enum postFilter(
                const physx::PxFilterData&,
                const physx::PxQueryHit&, 
                const physx::PxShape* shape, 
                const physx::PxRigidActor* actor)
                override {
				(void)shape;
				(void)actor;
                // This can be used for more detailed filtering after a hit is found,
                // but for now, we'll just treat any hit from preFilter as final.
                return physx::PxQueryHitType::eBLOCK;
            }
        };


        bool PhysicsEngine::CapsuleSweepSingle(
            const SharedVec3& start_pos, const SharedQuaternion& orientation,
            float radius, float half_height, const SharedVec3& unit_direction,
            float max_distance, HitResult& out_hit_result,
            physx::PxRigidActor* actor_to_ignore,
            const physx::PxQueryFilterData& filter_data_from_caller,
            physx::PxQueryFilterCallback* filter_callback_override
        ) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_scene) { RF_PHYSICS_ERROR("CapsuleSweepSingle: Scene not initialized."); return false; }
            if (max_distance <= 0.0f) { return false; }

            physx::PxTransform initial_pose(ToPxVec3(start_pos), ToPxQuat(orientation));
            physx::PxCapsuleGeometry capsule_geometry(radius, half_height);
            physx::PxVec3 sweep_direction_px = ToPxVec3(unit_direction);
            physx::PxSweepBuffer sweep_buffer; // For single closest blocking hit

            RiftStepSweepQueryFilterCallback default_callback(actor_to_ignore);
            physx::PxQueryFilterCallback* final_callback = filter_callback_override ? filter_callback_override : &default_callback;

            physx::PxHitFlags hit_flags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL | physx::PxHitFlag::eFACE_INDEX;

            bool hit_found = m_scene->sweep(capsule_geometry, initial_pose, sweep_direction_px, max_distance,
                sweep_buffer, hit_flags, filter_data_from_caller, final_callback);

            if (hit_found && sweep_buffer.hasBlock) {
                const physx::PxSweepHit& blocking_hit = sweep_buffer.block;
                out_hit_result.hit_actor = blocking_hit.actor;
                out_hit_result.hit_shape = blocking_hit.shape;
                out_hit_result.hit_point = FromPxVec3(blocking_hit.position);
                out_hit_result.hit_normal = FromPxVec3(blocking_hit.normal);
                out_hit_result.distance = blocking_hit.distance;
                out_hit_result.hit_face_index = blocking_hit.faceIndex;
                out_hit_result.hit_entity_id = (blocking_hit.actor && blocking_hit.actor->userData) ? reinterpret_cast<uint64_t>(blocking_hit.actor->userData) : 0;
                return true;
            }

            out_hit_result = HitResult(); // Clear if no blocking hit
            return false;
        }

        bool PhysicsEngine::RaycastSingle(
            const SharedVec3& start, const SharedVec3& unit_direction, float max_distance,
            HitResult& out_hit, const physx::PxQueryFilterData& filter_data,
            physx::PxQueryFilterCallback* filter_callback
        ) {
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_scene) { RF_PHYSICS_ERROR("RaycastSingle: Scene not initialized."); return false; }
            if (max_distance <= 0.0f) { return false; }

            physx::PxRaycastBuffer hit_buffer;
            physx::PxHitFlags hit_flags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL | physx::PxHitFlag::eFACE_INDEX;

            bool did_hit = m_scene->raycast(ToPxVec3(start), ToPxVec3(unit_direction), max_distance, hit_buffer, hit_flags, filter_data, filter_callback);

            if (did_hit && hit_buffer.hasBlock) {
                const physx::PxRaycastHit& block = hit_buffer.block;
                out_hit.hit_actor = block.actor;
                out_hit.hit_shape = block.shape;
                out_hit.hit_entity_id = (block.actor && block.actor->userData) ? reinterpret_cast<uint64_t>(block.actor->userData) : 0;
                out_hit.hit_point = FromPxVec3(block.position);
                out_hit.hit_normal = FromPxVec3(block.normal);
                out_hit.distance = block.distance;
                out_hit.hit_face_index = block.faceIndex;
                return true;
            }
            out_hit = HitResult();
            return false;
        }

        std::vector<HitResult> PhysicsEngine::RaycastMultiple(
            const SharedVec3& start, const SharedVec3& unit_direction, float max_distance,
            uint32_t max_hits, const physx::PxQueryFilterData& filter_data,
            physx::PxQueryFilterCallback* filter_callback
        ) {
            std::vector<HitResult> results;
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_scene || max_hits == 0 || max_distance <= 0.0f) { return results; }

            std::vector<physx::PxRaycastHit> touches(max_hits);
            physx::PxRaycastBuffer hit_buffer(touches.data(), max_hits);
            physx::PxHitFlags hit_flags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL | physx::PxHitFlag::eFACE_INDEX | physx::PxHitFlag::eMESH_MULTIPLE;

            if (m_scene->raycast(ToPxVec3(start), ToPxVec3(unit_direction), max_distance, hit_buffer, hit_flags, filter_data, filter_callback)) {
                results.reserve(hit_buffer.getNbAnyHits());
                for (physx::PxU32 i = 0; i < hit_buffer.getNbAnyHits(); ++i) {
                    const physx::PxRaycastHit& touch = hit_buffer.getAnyHit(i);
                    HitResult res;
                    res.hit_actor = touch.actor;
                    res.hit_shape = touch.shape;
                    res.hit_entity_id = (touch.actor && touch.actor->userData) ? reinterpret_cast<uint64_t>(touch.actor->userData) : 0;
                    res.hit_point = FromPxVec3(touch.position);
                    res.hit_normal = FromPxVec3(touch.normal);
                    res.distance = touch.distance;
                    res.hit_face_index = touch.faceIndex;
                    results.push_back(res);
                }
            }
            return results;
        }

        std::vector<HitResult> PhysicsEngine::OverlapMultiple(
            const physx::PxGeometry& geometry, const physx::PxTransform& pose,
            uint32_t max_hits, const physx::PxQueryFilterData& filter_data,
            physx::PxQueryFilterCallback* filter_callback
        ) {
            std::vector<HitResult> results;
            std::lock_guard<std::mutex> lock(m_physicsMutex);
            if (!m_scene || max_hits == 0) { return results; }

            std::vector<physx::PxOverlapHit> touches(max_hits);
            physx::PxOverlapBuffer hit_buffer(touches.data(), max_hits);

            if (m_scene->overlap(geometry, pose, hit_buffer, filter_data, filter_callback)) {
                results.reserve(hit_buffer.getNbAnyHits());
                for (physx::PxU32 i = 0; i < hit_buffer.getNbAnyHits(); ++i) {
                    const physx::PxOverlapHit& touch = hit_buffer.getAnyHit(i);
                    HitResult res;
                    res.hit_actor = touch.actor;
                    res.hit_shape = touch.shape;
                    res.hit_entity_id = (touch.actor && touch.actor->userData) ? reinterpret_cast<uint64_t>(touch.actor->userData) : 0;
                    res.hit_face_index = touch.faceIndex;
                    results.push_back(res);
                }
            }
            return results;
        }

    } // namespace Physics
} // namespace RiftForged