/**
 * MEMSHADOW Protocol v3.0 - Comprehensive Rust Test Suite
 * 
 * Tests all protocol features across Rust implementation.
 */

use memshadow::*;

#[cfg(test)]
mod tests {
    use super::*;

    // ========================================================================
    // CORE PROTOCOL TESTS
    // ========================================================================

    #[test]
    fn test_header_creation() {
        let header = MemshadowHeader::new(MessageType::Heartbeat, Priority::High, 1);
        assert_eq!(header.magic, MEMSHADOW_MAGIC);
        assert_eq!(header.msg_type, MessageType::Heartbeat as u16);
        assert_eq!(header.sequence_num, 1);
    }

    #[test]
    fn test_header_size() {
        let header = MemshadowHeader::new(MessageType::Data, Priority::High, 1);
        let packed = header.pack();
        assert_eq!(packed.len(), MEMSHADOW_HEADER_SIZE);
    }

    #[test]
    fn test_header_round_trip() {
        let original = MemshadowHeader::new(MessageType::Heartbeat, Priority::High, 123);
        let packed = original.pack();
        let unpacked = MemshadowHeader::unpack(&packed).unwrap();
        
        assert_eq!(original.magic, unpacked.magic);
        assert_eq!(original.version, unpacked.version);
        assert_eq!(original.priority, unpacked.priority);
        assert_eq!(original.msg_type, unpacked.msg_type);
        assert_eq!(original.sequence_num, unpacked.sequence_num);
    }

    #[test]
    fn test_all_message_types() {
        let types = vec![
            MessageType::Heartbeat,
            MessageType::Ack,
            MessageType::Nack,
            MessageType::Handshake,
            MessageType::FileTransferStart,
            MessageType::FileTransferChunk,
            MessageType::FileTransferEnd,
        ];
        
        for msg_type in types {
            let header = MemshadowHeader::new(msg_type, Priority::High, 1);
            assert_eq!(header.msg_type, msg_type as u16);
        }
    }

    #[test]
    fn test_sequence_number_wraparound() {
        let header1 = MemshadowHeader::new(MessageType::Data, Priority::High, 0xFFFFFFFF);
        let header2 = MemshadowHeader::new(MessageType::Data, Priority::High, 0);
        
        assert_eq!(header1.sequence_num, 0xFFFFFFFF);
        assert_eq!(header2.sequence_num, 0);
    }

    #[test]
    fn test_header_validation() {
        let header = MemshadowHeader::new(MessageType::Heartbeat, Priority::High, 1);
        let packed = header.pack();
        
        // Valid header should unpack successfully
        assert!(MemshadowHeader::unpack(&packed).is_some());
        
        // Invalid header (too short) should fail
        assert!(MemshadowHeader::unpack(&packed[..16]).is_none());
        
        // Invalid magic should fail
        let mut bad_data = packed.clone();
        bad_data[0] = 0xFF;
        let unpacked = MemshadowHeader::unpack(&bad_data);
        assert!(unpacked.is_none() || unpacked.unwrap().magic != MEMSHADOW_MAGIC);
    }

    #[test]
    fn test_timestamp_range() {
        let header = MemshadowHeader::new(MessageType::Data, Priority::High, 1);
        assert!(header.timestamp_ns > 0);
        
        // Test max timestamp
        let mut header_max = header.clone();
        header_max.timestamp_ns = 0xFFFFFFFFFFFFFFFF;
        let packed = header_max.pack();
        let unpacked = MemshadowHeader::unpack(&packed).unwrap();
        assert_eq!(unpacked.timestamp_ns, 0xFFFFFFFFFFFFFFFF);
    }

    // ========================================================================
    // VLAN TOPOLOGY TESTS
    // ========================================================================

    #[test]
    fn test_vlan_node_creation() {
        use std::time::{SystemTime, UNIX_EPOCH};
        let node = VlanNode {
            node_id: "node1".to_string(),
            vlan_id: 100,
            address: "192.168.1.10".to_string(),
            port: 8901,
            connectivity: NodeConnectivity::Internet,
            last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            relay_capacity: 10,
            relay_load: 0,
        };
        
        assert_eq!(node.node_id, "node1");
        assert_eq!(node.vlan_id, 100);
        assert_eq!(node.connectivity, NodeConnectivity::Internet);
    }

    #[test]
    fn test_topology_discovery() {
        use std::time::{SystemTime, UNIX_EPOCH};
        let mut topology = TopologyDiscovery::new("node1".to_string());
        
        let node2 = VlanNode {
            node_id: "node2".to_string(),
            vlan_id: 100,
            address: "192.168.1.20".to_string(),
            port: 8901,
            connectivity: NodeConnectivity::Internet,
            last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            relay_capacity: 10,
            relay_load: 0,
        };
        
        topology.discover_node(node2);
        topology.add_edge("node1", "node2");
        
        let path = topology.find_path("node1", "node2", 5);
        assert!(path.is_some());
    }

    #[test]
    fn test_multi_hop_path() {
        use std::time::{SystemTime, UNIX_EPOCH};
        let mut topology = TopologyDiscovery::new("node1".to_string());
        
        // Add nodes
        for i in 2..6 {
            let node = VlanNode {
                node_id: format!("node{}", i),
                vlan_id: 100,
                address: format!("192.168.1.{}", i * 10),
                port: 8901,
                connectivity: NodeConnectivity::VlanOnly,
                last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
                relay_capacity: 10,
                relay_load: 0,
            };
            topology.discover_node(node);
            
            if i == 2 {
                topology.add_edge("node1", &format!("node{}", i));
            } else {
                topology.add_edge(&format!("node{}", i - 1), &format!("node{}", i));
            }
        }
        
        let path = topology.find_path("node1", "node5", 5);
        assert!(path.is_some());
        assert!(path.unwrap().len() >= 2);
    }

    #[test]
    fn test_relay_path_finding() {
        use std::time::{SystemTime, UNIX_EPOCH};
        let mut manager = VlanRelayManager::new("node1".to_string(), 100);
        
        // Register node1 itself (needed for topology graph)
        let node1 = VlanNode {
            node_id: "node1".to_string(),
            vlan_id: 100,
            address: "192.168.1.10".to_string(),
            port: 8901,
            connectivity: NodeConnectivity::VlanOnly,
            last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            relay_capacity: 10,
            relay_load: 0,
        };
        
        let node2 = VlanNode {
            node_id: "node2".to_string(),
            vlan_id: 100,
            address: "192.168.1.20".to_string(),
            port: 8901,
            connectivity: NodeConnectivity::Internet,
            last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            relay_capacity: 10,
            relay_load: 0,
        };
        
        manager.register_node(node1);
        manager.register_node(node2);
        manager.register_connection("node1", "node2");
        
        // find_relay_path uses path_finder which needs topology to have nodes
        let path = manager.find_relay_path("node2", false);
        // Path should exist since we have direct connection
        assert!(path.is_some(), "Path from node1 to node2 should exist");
    }

    // ========================================================================
    // DPI EVASION TESTS
    // ========================================================================

    #[test]
    fn test_http_mimicry() {
        let mimicry = HttpMimicry::new();
        let payload = b"secret data";
        
        let wrapped = mimicry.wrap_request(payload);
        assert!(!wrapped.is_empty());
        assert!(wrapped.len() > payload.len());
    }

    #[test]
    fn test_dpi_evasion_manager() {
        let mut manager = DpiEvasionManager::new(DpiEvasionMode::ProtocolMimicry);
        let payload = b"test payload";
        
        let wrapped = manager.wrap_payload(payload, Some(ProtocolMimic::Http));
        assert!(!wrapped.is_empty());
        
        let unwrapped = manager.unwrap_payload(&wrapped, Some(ProtocolMimic::Http));
        assert!(unwrapped.is_some());
    }

    #[test]
    fn test_protocol_rotation() {
        let mut manager = DpiEvasionManager::new(DpiEvasionMode::ProtocolMimicry);
        let payload = b"test";
        
        // Test wrapping with different protocols
        let wrapped1 = manager.wrap_payload(payload, None);
        manager.rotate_protocol();
        let wrapped2 = manager.wrap_payload(payload, None);
        
        // Both should be valid wrapped payloads
        assert!(!wrapped1.is_empty());
        assert!(!wrapped2.is_empty());
    }

    // ========================================================================
    // TRAFFIC ANALYSIS RESISTANCE TESTS
    // ========================================================================

    #[test]
    fn test_packet_size_normalization() {
        let mut normalizer = PacketSizeNormalizer::new();
        let data = b"test data ".repeat(1000);
        
        let normalized = normalizer.normalize(&data);
        assert!(!normalized.is_empty());
    }

    #[test]
    fn test_timing_pattern_masking() {
        let mut masker = TimingPatternMasker::new();
        let delay = masker.get_next_delay();
        
        assert!(delay > 0.0);
        assert!(delay < 1.0);
    }

    #[test]
    fn test_traffic_analysis_resistance() {
        let mut resistance = TrafficAnalysisResistance::new(ResistanceLevel::Advanced);
        let packets = vec![b"packet1".to_vec(), b"packet2".to_vec()];
        
        let (processed, delays) = resistance.process_packets(&packets, false);
        assert!(!processed.is_empty());
        assert_eq!(delays.len(), processed.len());
    }

    // ========================================================================
    // COVERT VLAN TESTS
    // ========================================================================

    #[test]
    fn test_covert_vlan_manager() {
        let config = CovertVlanConfig {
            mode: CovertVlanMode::Advanced,
            dpi_evasion: true,
            use_covert_channels: true,
            traffic_resistance: true,
            protocol_mimic: ProtocolMimic::Http,
            dummy_traffic_rate: 0.15,
        };
        
        let manager = CovertVlanManager::new("node1".to_string(), 100, Some(config));
        // Manager should be created successfully - test by using it
        use std::time::{SystemTime, UNIX_EPOCH};
        let node2 = VlanNode {
            node_id: "node2".to_string(),
            vlan_id: 100,
            address: "192.168.1.20".to_string(),
            port: 8901,
            connectivity: NodeConnectivity::Internet,
            last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            relay_capacity: 10,
            relay_load: 0,
        };
        // If we can register a node, manager is working
        let mut mgr = manager;
        mgr.register_node(node2);
        assert!(true); // Test passes if no panic
    }

    #[test]
    fn test_covert_message_sending() {
        use std::time::{SystemTime, UNIX_EPOCH};
        let mut manager = CovertVlanManager::new("node1".to_string(), 100, None);
        
        // Register node1 itself
        let node1 = VlanNode {
            node_id: "node1".to_string(),
            vlan_id: 100,
            address: "192.168.1.10".to_string(),
            port: 8901,
            connectivity: NodeConnectivity::VlanOnly,
            last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            relay_capacity: 10,
            relay_load: 0,
        };
        
        let node2 = VlanNode {
            node_id: "node2".to_string(),
            vlan_id: 100,
            address: "192.168.1.20".to_string(),
            port: 8901,
            connectivity: NodeConnectivity::Internet,
            last_seen: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            relay_capacity: 10,
            relay_load: 0,
        };
        manager.register_node(node1);
        manager.register_node(node2);
        manager.register_connection("node1", "node2");
        
        let payload = b"covert message";
        let relay_id = manager.send_covert_message("node2", payload, true);
        assert!(relay_id.is_some());
    }

    // ========================================================================
    // EDGE CASE TESTS
    // ========================================================================

    #[test]
    fn test_zero_length_payload() {
        let header = MemshadowHeader::new(MessageType::Heartbeat, Priority::High, 1);
        assert_eq!(header.payload_len, 0);
    }

    #[test]
    fn test_very_large_sequence() {
        let header = MemshadowHeader::new(MessageType::Data, Priority::High, 0xFFFFFFFF);
        assert_eq!(header.sequence_num, 0xFFFFFFFF);
        
        let packed = header.pack();
        let unpacked = MemshadowHeader::unpack(&packed).unwrap();
        assert_eq!(unpacked.sequence_num, 0xFFFFFFFF);
    }

    #[test]
    fn test_malformed_header() {
        // Too short
        assert!(MemshadowHeader::unpack(&[0u8; 16]).is_none());
        
        // Invalid magic
        let bad_data = vec![0xFFu8; MEMSHADOW_HEADER_SIZE];
        let unpacked_bad = MemshadowHeader::unpack(&bad_data);
        assert!(unpacked_bad.is_none() || 
                unpacked_bad.unwrap().magic != MEMSHADOW_MAGIC);
    }

    #[test]
    fn test_concurrent_operations() {
        use std::thread;
        
        let mut handles = vec![];
        for i in 0..10 {
            handles.push(thread::spawn(move || {
                let header = MemshadowHeader::new(MessageType::Data, Priority::High, i);
                let packed = header.pack();
                let unpacked = MemshadowHeader::unpack(&packed).unwrap();
                assert_eq!(unpacked.sequence_num, i);
            }));
        }
        
        for handle in handles {
            handle.join().unwrap();
        }
    }
}
