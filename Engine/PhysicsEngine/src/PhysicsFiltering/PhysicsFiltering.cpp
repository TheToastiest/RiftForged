// File: Engine/PhysicsEngine/src/PhysicsFiltering.cpp
// Copyright (c) 2025-2028 RiftForged Game Development Team

#include <RiftForged/Physics/PhysicsEngine/PhysicsEngine.h>

// Note: No other includes are needed here, as PhysicsEngine.h transitively
// includes PhysicsTypes.h, which brings in all the necessary PhysX types.

namespace RiftForged {
    namespace Physics {

        // This is a free function, not a member of the PhysicsEngine class.
        // It defines the global collision behavior for the entire scene.
        physx::PxFilterFlags CustomFilterShader(
            physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
        {
            (void)constantBlock;      
            (void)constantBlockSize;  
            // Let triggers through
            if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1)) {
                pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
                return physx::PxFilterFlag::eDEFAULT;
            }

            // Generate contacts for everything else
            pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

            // Example of custom interactions:
            // Prevent projectiles from colliding with each other
            EPhysicsObjectType type0 = static_cast<EPhysicsObjectType>(filterData0.word0);
            EPhysicsObjectType type1 = static_cast<EPhysicsObjectType>(filterData1.word0);

            bool type0IsProjectile = (type0 == EPhysicsObjectType::MAGIC_PROJECTILE || type0 == EPhysicsObjectType::PROJECTILE);
            bool type1IsProjectile = (type1 == EPhysicsObjectType::MAGIC_PROJECTILE || type1 == EPhysicsObjectType::PROJECTILE);

            if (type0IsProjectile && type1IsProjectile) {
                return physx::PxFilterFlag::eSUPPRESS;
            }

            // Enable contact callbacks for projectiles hitting characters/enemies
            bool type0IsTargetable = (type0 == EPhysicsObjectType::PLAYER_CHARACTER || type0 == EPhysicsObjectType::SMALL_ENEMY || type0 == EPhysicsObjectType::MEDIUM_ENEMY || type0 == EPhysicsObjectType::LARGE_ENEMY || type0 == EPhysicsObjectType::HUGE_ENEMY || type0 == EPhysicsObjectType::RAID_BOSS || type0 == EPhysicsObjectType::VAELITH);
            bool type1IsTargetable = (type1 == EPhysicsObjectType::PLAYER_CHARACTER || type1 == EPhysicsObjectType::SMALL_ENEMY || type1 == EPhysicsObjectType::MEDIUM_ENEMY || type1 == EPhysicsObjectType::LARGE_ENEMY || type1 == EPhysicsObjectType::HUGE_ENEMY || type1 == EPhysicsObjectType::RAID_BOSS || type1 == EPhysicsObjectType::VAELITH);

            if ((type0IsProjectile && type1IsTargetable) || (type1IsProjectile && type0IsTargetable)) {
                pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
                pairFlags |= physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;
            }

            // Enable contact callbacks for melee weapons hitting characters/enemies
            bool type0IsMelee = (type0 == EPhysicsObjectType::MELEE_WEAPON);
            bool type1IsMelee = (type1 == EPhysicsObjectType::MELEE_WEAPON);

            if ((type0IsMelee && type1IsTargetable) || (type1IsMelee && type0IsTargetable)) {
                pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
            }

            return physx::PxFilterFlag::eDEFAULT;
        }


        // This is a private helper member function of the PhysicsEngine class.
        void PhysicsEngine::SetupShapeFiltering(physx::PxShape* shape, const CollisionFilterData& filter_data) {
            if (shape) {
                physx::PxFilterData px_filter_data;
                px_filter_data.word0 = filter_data.word0;
                px_filter_data.word1 = filter_data.word1;
                px_filter_data.word2 = filter_data.word2;
                px_filter_data.word3 = filter_data.word3;

                // Apply the filter data to both simulation and scene queries
                shape->setQueryFilterData(px_filter_data);
                shape->setSimulationFilterData(px_filter_data);
            }
        }

    } // namespace Physics
} // namespace RiftForged