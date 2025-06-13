#pragma once

// Enable GLM experimental features FIRST
#define GLM_ENABLE_EXPERIMENTAL

// GLM Headers
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp> // For glm::pi(), glm::epsilon()
#include <glm/gtx/norm.hpp>     // For glm::distance2, glm::length2
#include <glm/gtx/vector_angle.hpp> // For potential future use, good to have with quaternions/vectors

// Standard Library
#include <cmath> // For std::sqrt, std::sin, std::cos, std::abs (though GLM often provides these)

// The FlatBuffers include for Vec3 & Quaternion is no longer strictly needed by this file
// if all math operations are to use GLM types. You might still need it elsewhere for
// serialization/deserialization with your network protocols.
// #include "../FlatBuffers/V0.0.5/riftforged_common_types_generated.h"

namespace RiftForged {
    namespace Utilities {
        namespace Math {

            // Type aliases for convenience using GLM
            using Vec3 = glm::vec3;
            using Quaternion = glm::quat;

            // --- Mathematical Constants ---
            const float PI_F = glm::pi<float>();
            const float DEG_TO_RAD_FACTOR = PI_F / 180.0f; // or use glm::radians() directly
            const float RAD_TO_DEG_FACTOR = 180.0f / PI_F; // or use glm::degrees() directly

            // Epsilon values for comparisons and normalization
            const float QUATERNION_NORMALIZATION_EPSILON_SQ = 0.00001f * 0.00001f; // Use squared epsilon for squared length
            const float VECTOR_NORMALIZATION_EPSILON_SQ = 0.00001f * 0.00001f;     // Use squared epsilon for squared length
            const float DEFAULT_VECTOR_CLOSE_EPSILON = 0.001f;
            const float DEFAULT_QUATERNION_DOT_EPSILON = 0.99999f; // For checking if quaternions are close (aligned)

            // --- Vector Operations ---

            // Magnitude of a Vec3
            inline float Magnitude(const Vec3& v) {
                return glm::length(v);
            }

            // Normalizes a Vec3, returning zero vector if magnitude is too small
            inline Vec3 NormalizeVector(const Vec3& v) {
                float mag_sq = glm::length2(v); // Use glm::length2 for squared magnitude (more efficient)
                if (mag_sq > VECTOR_NORMALIZATION_EPSILON_SQ) {
                    return v / std::sqrt(mag_sq); // glm::normalize(v) could also be used but check its behavior for zero vectors
                }
                return Vec3(0.0f, 0.0f, 0.0f); // Return zero vector
            }

            // AddVectors (using GLM's operator overloading)
            inline Vec3 AddVectors(const Vec3& v1, const Vec3& v2) {
                return v1 + v2;
            }

            // ScaleVector (using GLM's operator overloading)
            inline Vec3 ScaleVector(const Vec3& v, float scalar) {
                return v * scalar;
            }

            // SubtractVectors (using GLM's operator overloading)
            inline Vec3 SubtractVectors(const Vec3& v1, const Vec3& v2) {
                return v1 - v2;
            }

            // DotProduct
            inline float DotProduct(const Vec3& v1, const Vec3& v2) {
                return glm::dot(v1, v2);
            }

            // DistanceSquared
            inline float DistanceSquared(const Vec3& v1, const Vec3& v2) {
                return glm::distance2(v1, v2); // from <glm/gtx/norm.hpp>
            }

            // Distance
            inline float Distance(const Vec3& v1, const Vec3& v2) {
                return glm::distance(v1, v2);
            }

            // AreVectorsClose
            inline bool AreVectorsClose(const Vec3& v1, const Vec3& v2, float epsilon = DEFAULT_VECTOR_CLOSE_EPSILON) {
                return DistanceSquared(v1, v2) < (epsilon * epsilon);
            }


            // --- Quaternion Operations ---

            // Normalize a Quaternion, returning identity if magnitude is too small
            inline Quaternion NormalizeQuaternion(const Quaternion& q) {
                float mag_sq = glm::length2(q); // q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w
                if (mag_sq > QUATERNION_NORMALIZATION_EPSILON_SQ) {
                    return glm::normalize(q); // glm::normalize handles this well for non-zero quaternions
                }
                return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); // Identity quaternion
            }

            // Create Quaternion from Angle-Axis (angle in degrees)
            inline Quaternion FromAngleAxis(float angle_degrees, const Vec3& axis) {
                Vec3 norm_axis = NormalizeVector(axis); // Ensure axis is normalized
                // If axis is zero vector after normalization, return identity quaternion
                if (glm::length2(norm_axis) < VECTOR_NORMALIZATION_EPSILON_SQ) {
                    return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); // Identity
                }
                return glm::angleAxis(glm::radians(angle_degrees), norm_axis);
            }

            // Multiply Quaternions (applies q1's rotation then q2's rotation)
            // In GLM, q2 * q1 means apply q1 then apply q2.
            // If your original function MultiplyQuaternions(qA, qB) computed qA * qB,
            // and this meant "apply qB, then apply qA", then GLM's qA * qB is a direct replacement.
            // This is typical for object_orientation = object_orientation * local_rotation.
            inline Quaternion MultiplyQuaternions(const Quaternion& q1, const Quaternion& q2) {
                return q1 * q2; // If q1 is existing orientation, q2 is local rotation: new_orientation = q1 * q2
                // If q1 is first rotation in a sequence, q2 is second: combined_rotation = q2 * q1
                // Assuming the former based on common usage (q_existing * q_local).
            }

            // Rotate a Vector by a Quaternion
            inline Vec3 RotateVectorByQuaternion(const Vec3& v, const Quaternion& q) {
                return q * v; // GLM directly supports this: q * v applies the rotation q to vector v
            }

            // Check if two quaternions are close (represent similar rotations)
            // q1 and -q1 represent the same rotation. Dot product will be -1 for -q1.
            inline bool AreQuaternionsClose(const Quaternion& q1, const Quaternion& q2, float dot_product_tolerance = DEFAULT_QUATERNION_DOT_EPSILON) {
                float dot_val = glm::dot(q1, q2);
                // If abs(dot) is close to 1, the angle between them is close to 0 or 180 (anti-parallel).
                // Anti-parallel quaternions (q and -q) represent the same rotation.
                return std::abs(dot_val) > dot_product_tolerance;
            }

            // Get World Forward Vector (assuming local +Y is forward)
            inline Vec3 GetWorldForwardVector(const Quaternion& orientation) {
                return orientation * Vec3(0.0f, 1.0f, 0.0f);
            }

            // Get World Right Vector (assuming local +X is right)
            inline Vec3 GetWorldRightVector(const Quaternion& orientation) {
                return orientation * Vec3(1.0f, 0.0f, 0.0f);
            }

            // Get World Up Vector (assuming local +Z is up)
            // Note: Many systems use +Y as Up. Adjust if your convention is different.
            inline Vec3 GetWorldUpVector(const Quaternion& orientation) {
                return orientation * Vec3(0.0f, 0.0f, 1.0f);
            }

            // TODO: Add other math utilities as needed (e.g., matrix operations, Lerp, Slerp from GLM)
            // For Slerp: glm::slerp(q1, q2, alpha)
            // For Lerp (vectors): glm::mix(v1, v2, alpha)

        } // namespace Math
    } // namespace Utilities
} // namespace RiftForged