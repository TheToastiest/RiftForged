// File: NetworkCommon.h
// RiftForged Game Development
// Copyright (c) 2023-2025 RiftForged Game Development Team

#pragma once

#include <optional>
#include <vector>
#include "flatbuffers/flatbuffers.h" // For flatbuffers::DetachedBuffer.

#include "NetworkEndpoint.h" // For RiftForged::Networking::NetworkEndpoint.

// Include the FlatBuffers generated S2C messages header.
// This is necessary to get the `S2C_UDP_Payload` enum.
#include "../FlatBuffers/Versioning/V0.0.4/riftforged_s2c_udp_messages_generated.h" // Assuming this is the correct path

namespace RiftForged {
    namespace Networking {

        struct S2C_Response {
            flatbuffers::DetachedBuffer data;
            UDP::S2C::S2C_UDP_Payload flatbuffer_payload_type;
            bool broadcast = false;
            NetworkEndpoint specific_recipient;

            S2C_Response() = default;

            S2C_Response(flatbuffers::DetachedBuffer&& data_buf,
                UDP::S2C::S2C_UDP_Payload payload_type,
                bool is_broadcast,
                const NetworkEndpoint& recipient_ep = NetworkEndpoint())
                : data(std::move(data_buf)),
                flatbuffer_payload_type(payload_type),
                broadcast(is_broadcast),
                specific_recipient(recipient_ep)
            {
            }
        };

    } // namespace Networking
} // namespace RiftForged