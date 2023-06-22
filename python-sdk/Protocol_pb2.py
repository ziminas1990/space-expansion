# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: Protocol.proto
"""Generated protocol buffer code."""
from google.protobuf.internal import builder as _builder
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import CommonTypes_pb2 as CommonTypes__pb2


DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n\x0eProtocol.proto\x12\x04spex\x1a\x11\x43ommonTypes.proto\"W\n\x0fISessionControl\x12\x13\n\theartbeat\x18\x01 \x01(\x08H\x00\x12\x0f\n\x05\x63lose\x18\x10 \x01(\x08H\x00\x12\x14\n\nclosed_ind\x18@ \x01(\x08H\x00\x42\x08\n\x06\x63hoice\"X\n\x0cIRootSession\x12 \n\x16new_commutator_session\x18\x01 \x01(\x08H\x00\x12\x1c\n\x12\x63ommutator_session\x18\x15 \x01(\rH\x00\x42\x08\n\x06\x63hoice\"\x85\x02\n\x0cIAccessPanel\x12\x30\n\x05login\x18\x01 \x01(\x0b\x32\x1f.spex.IAccessPanel.LoginRequestH\x00\x12:\n\x0e\x61\x63\x63\x65ss_granted\x18\x15 \x01(\x0b\x32 .spex.IAccessPanel.AccessGrantedH\x00\x12\x19\n\x0f\x61\x63\x63\x65ss_rejected\x18\x16 \x01(\tH\x00\x1a/\n\x0cLoginRequest\x12\r\n\x05login\x18\x01 \x01(\t\x12\x10\n\x08password\x18\x02 \x01(\t\x1a\x31\n\rAccessGranted\x12\x0c\n\x04port\x18\x01 \x01(\r\x12\x12\n\nsession_id\x18\x02 \x01(\rB\x08\n\x06\x63hoice\"\x87\x03\n\x07IEngine\x12\x1b\n\x11specification_req\x18\x01 \x01(\x08H\x00\x12\x33\n\rchange_thrust\x18\x02 \x01(\x0b\x32\x1a.spex.IEngine.ChangeThrustH\x00\x12\x14\n\nthrust_req\x18\x03 \x01(\x08H\x00\x12\x34\n\rspecification\x18\x15 \x01(\x0b\x32\x1b.spex.IEngine.SpecificationH\x00\x12-\n\x06thrust\x18\x16 \x01(\x0b\x32\x1b.spex.IEngine.CurrentThrustH\x00\x1a#\n\rSpecification\x12\x12\n\nmax_thrust\x18\x01 \x01(\r\x1aI\n\x0c\x43hangeThrust\x12\t\n\x01x\x18\x01 \x01(\x01\x12\t\n\x01y\x18\x02 \x01(\x01\x12\x0e\n\x06thrust\x18\x04 \x01(\r\x12\x13\n\x0b\x64uration_ms\x18\x05 \x01(\r\x1a\x35\n\rCurrentThrust\x12\t\n\x01x\x18\x01 \x01(\x01\x12\t\n\x01y\x18\x02 \x01(\x01\x12\x0e\n\x06thrust\x18\x04 \x01(\rB\x08\n\x06\x63hoice\"\xae\x01\n\x05IShip\x12\x13\n\tstate_req\x18\x01 \x01(\x08H\x00\x12\x11\n\x07monitor\x18\x02 \x01(\rH\x00\x12\"\n\x05state\x18\x15 \x01(\x0b\x32\x11.spex.IShip.StateH\x00\x1aO\n\x05State\x12$\n\x06weight\x18\x01 \x01(\x0b\x32\x14.spex.OptionalDouble\x12 \n\x08position\x18\x02 \x01(\x0b\x32\x0e.spex.PositionB\x08\n\x06\x63hoice\"S\n\x0bINavigation\x12\x16\n\x0cposition_req\x18\x01 \x01(\x08H\x00\x12\"\n\x08position\x18\x15 \x01(\x0b\x32\x0e.spex.PositionH\x00\x42\x08\n\x06\x63hoice\"\xf9\x04\n\x11ICelestialScanner\x12\x1b\n\x11specification_req\x18\x01 \x01(\x08H\x00\x12,\n\x04scan\x18\x02 \x01(\x0b\x32\x1c.spex.ICelestialScanner.ScanH\x00\x12>\n\rspecification\x18\x15 \x01(\x0b\x32%.spex.ICelestialScanner.SpecificationH\x00\x12>\n\x0fscanning_report\x18\x16 \x01(\x0b\x32#.spex.ICelestialScanner.ScanResultsH\x00\x12\x39\n\x0fscanning_failed\x18\x17 \x01(\x0e\x32\x1e.spex.ICelestialScanner.StatusH\x00\x1a\x42\n\rSpecification\x12\x15\n\rmax_radius_km\x18\x01 \x01(\r\x12\x1a\n\x12processing_time_us\x18\x02 \x01(\r\x1a<\n\x04Scan\x12\x1a\n\x12scanning_radius_km\x18\x01 \x01(\r\x12\x18\n\x10minimal_radius_m\x18\x02 \x01(\r\x1aS\n\x0c\x41steroidInfo\x12\n\n\x02id\x18\x01 \x01(\r\x12\t\n\x01x\x18\x02 \x01(\x01\x12\t\n\x01y\x18\x03 \x01(\x01\x12\n\n\x02vx\x18\x04 \x01(\x01\x12\n\n\x02vy\x18\x05 \x01(\x01\x12\t\n\x01r\x18\x06 \x01(\x01\x1aT\n\x0bScanResults\x12\x37\n\tasteroids\x18\x01 \x03(\x0b\x32$.spex.ICelestialScanner.AsteroidInfo\x12\x0c\n\x04left\x18\x02 \x01(\r\"\'\n\x06Status\x12\x0b\n\x07SUCCESS\x10\x00\x12\x10\n\x0cSCANNER_BUSY\x10\x01\x42\x08\n\x06\x63hoice\"\xc8\x02\n\x0fIPassiveScanner\x12\x1b\n\x11specification_req\x18\x01 \x01(\x08H\x00\x12\x11\n\x07monitor\x18\x02 \x01(\x08H\x00\x12<\n\rspecification\x18\x15 \x01(\x0b\x32#.spex.IPassiveScanner.SpecificationH\x00\x12\x15\n\x0bmonitor_ack\x18\x16 \x01(\x08H\x00\x12.\n\x06update\x18\x17 \x01(\x0b\x32\x1c.spex.IPassiveScanner.UpdateH\x00\x1aG\n\rSpecification\x12\x1a\n\x12scanning_radius_km\x18\x01 \x01(\r\x12\x1a\n\x12max_update_time_ms\x18\x02 \x01(\r\x1a-\n\x06Update\x12#\n\x05items\x18\x01 \x03(\x0b\x32\x14.spex.PhysicalObjectB\x08\n\x06\x63hoice\"\x8a\x04\n\x10IAsteroidScanner\x12\x1b\n\x11specification_req\x18\x01 \x01(\x08H\x00\x12\x17\n\rscan_asteroid\x18\x02 \x01(\rH\x00\x12=\n\rspecification\x18\x15 \x01(\x0b\x32$.spex.IAsteroidScanner.SpecificationH\x00\x12\x38\n\x0fscanning_status\x18\x16 \x01(\x0e\x32\x1d.spex.IAsteroidScanner.StatusH\x00\x12>\n\x11scanning_finished\x18\x17 \x01(\x0b\x32!.spex.IAsteroidScanner.ScanResultH\x00\x1a?\n\rSpecification\x12\x14\n\x0cmax_distance\x18\x01 \x01(\r\x12\x18\n\x10scanning_time_ms\x18\x02 \x01(\r\x1ay\n\nScanResult\x12\x13\n\x0b\x61steroid_id\x18\x01 \x01(\r\x12\x0e\n\x06weight\x18\x02 \x01(\x01\x12\x16\n\x0emetals_percent\x18\x03 \x01(\x01\x12\x13\n\x0bice_percent\x18\x04 \x01(\x01\x12\x19\n\x11silicates_percent\x18\x05 \x01(\x01\"A\n\x06Status\x12\x0f\n\x0bIN_PROGRESS\x10\x00\x12\x10\n\x0cSCANNER_BUSY\x10\x01\x12\x14\n\x10\x41STEROID_TOO_FAR\x10\x02\x42\x08\n\x06\x63hoice\"\xc6\x07\n\x12IResourceContainer\x12\x15\n\x0b\x63ontent_req\x18\x01 \x01(\x08H\x00\x12\x13\n\topen_port\x18\x02 \x01(\rH\x00\x12\x14\n\nclose_port\x18\x03 \x01(\x08H\x00\x12\x35\n\x08transfer\x18\x04 \x01(\x0b\x32!.spex.IResourceContainer.TransferH\x00\x12\x11\n\x07monitor\x18\x05 \x01(\x08H\x00\x12\x33\n\x07\x63ontent\x18\x15 \x01(\x0b\x32 .spex.IResourceContainer.ContentH\x00\x12\x15\n\x0bport_opened\x18\x16 \x01(\rH\x00\x12;\n\x10open_port_failed\x18\x17 \x01(\x0e\x32\x1f.spex.IResourceContainer.StatusH\x00\x12<\n\x11\x63lose_port_status\x18\x18 \x01(\x0e\x32\x1f.spex.IResourceContainer.StatusH\x00\x12:\n\x0ftransfer_status\x18\x19 \x01(\x0e\x32\x1f.spex.IResourceContainer.StatusH\x00\x12-\n\x0ftransfer_report\x18\x1a \x01(\x0b\x32\x12.spex.ResourceItemH\x00\x12<\n\x11transfer_finished\x18\x1b \x01(\x0e\x32\x1f.spex.IResourceContainer.StatusH\x00\x1aN\n\x07\x43ontent\x12\x0e\n\x06volume\x18\x01 \x01(\r\x12\x0c\n\x04used\x18\x02 \x01(\x01\x12%\n\tresources\x18\x03 \x03(\x0b\x32\x12.spex.ResourceItem\x1aU\n\x08Transfer\x12\x0f\n\x07port_id\x18\x01 \x01(\r\x12\x12\n\naccess_key\x18\x02 \x01(\r\x12$\n\x08resource\x18\x03 \x01(\x0b\x32\x12.spex.ResourceItem\"\x82\x02\n\x06Status\x12\x0b\n\x07SUCCESS\x10\x00\x12\x12\n\x0eINTERNAL_ERROR\x10\x01\x12\x15\n\x11PORT_ALREADY_OPEN\x10\x02\x12\x15\n\x11PORT_DOESNT_EXIST\x10\x03\x12\x16\n\x12PORT_IS_NOT_OPENED\x10\x04\x12\x18\n\x14PORT_HAS_BEEN_CLOSED\x10\x05\x12\x16\n\x12INVALID_ACCESS_KEY\x10\x06\x12\x19\n\x15INVALID_RESOURCE_TYPE\x10\x07\x12\x10\n\x0cPORT_TOO_FAR\x10\x08\x12\x18\n\x14TRANSFER_IN_PROGRESS\x10\t\x12\x18\n\x14NOT_ENOUGH_RESOURCES\x10\nB\x08\n\x06\x63hoice\"\xf7\x05\n\x0eIAsteroidMiner\x12\x1b\n\x11specification_req\x18\x01 \x01(\x08H\x00\x12\x17\n\rbind_to_cargo\x18\x02 \x01(\tH\x00\x12\x16\n\x0cstart_mining\x18\x03 \x01(\rH\x00\x12\x15\n\x0bstop_mining\x18\x04 \x01(\x08H\x00\x12;\n\rspecification\x18\x15 \x01(\x0b\x32\".spex.IAsteroidMiner.SpecificationH\x00\x12;\n\x14\x62ind_to_cargo_status\x18\x16 \x01(\x0e\x32\x1b.spex.IAsteroidMiner.StatusH\x00\x12:\n\x13start_mining_status\x18\x17 \x01(\x0e\x32\x1b.spex.IAsteroidMiner.StatusH\x00\x12(\n\rmining_report\x18\x18 \x01(\x0b\x32\x0f.spex.ResourcesH\x00\x12\x38\n\x11mining_is_stopped\x18\x19 \x01(\x0e\x32\x1b.spex.IAsteroidMiner.StatusH\x00\x12\x39\n\x12stop_mining_status\x18\x1a \x01(\x0e\x32\x1b.spex.IAsteroidMiner.StatusH\x00\x1aU\n\rSpecification\x12\x14\n\x0cmax_distance\x18\x01 \x01(\r\x12\x15\n\rcycle_time_ms\x18\x02 \x01(\r\x12\x17\n\x0fyield_per_cycle\x18\x03 \x01(\r\"\xc9\x01\n\x06Status\x12\x0b\n\x07SUCCESS\x10\x00\x12\x12\n\x0eINTERNAL_ERROR\x10\x01\x12\x19\n\x15\x41STEROID_DOESNT_EXIST\x10\x02\x12\x11\n\rMINER_IS_BUSY\x10\x03\x12\x11\n\rMINER_IS_IDLE\x10\x04\x12\x14\n\x10\x41STEROID_TOO_FAR\x10\x05\x12\x16\n\x12NO_SPACE_AVAILABLE\x10\x06\x12\x16\n\x12NOT_BOUND_TO_CARGO\x10\x07\x12\x17\n\x13INTERRUPTED_BY_USER\x10\x08\x42\x08\n\x06\x63hoice\"\xa7\x02\n\x12IBlueprintsLibrary\x12\x1d\n\x13\x62lueprints_list_req\x18\x01 \x01(\tH\x00\x12\x17\n\rblueprint_req\x18\x02 \x01(\tH\x00\x12*\n\x0f\x62lueprints_list\x18\x14 \x01(\x0b\x32\x0f.spex.NamesListH\x00\x12$\n\tblueprint\x18\x15 \x01(\x0b\x32\x0f.spex.BlueprintH\x00\x12\x39\n\x0e\x62lueprint_fail\x18\x16 \x01(\x0e\x32\x1f.spex.IBlueprintsLibrary.StatusH\x00\"B\n\x06Status\x12\x0b\n\x07SUCCESS\x10\x00\x12\x12\n\x0eINTERNAL_ERROR\x10\x01\x12\x17\n\x13\x42LUEPRINT_NOT_FOUND\x10\x02\x42\x08\n\x06\x63hoice\"\xbd\x06\n\tIShipyard\x12\x1b\n\x11specification_req\x18\x01 \x01(\x08H\x00\x12\x17\n\rbind_to_cargo\x18\x03 \x01(\tH\x00\x12\x31\n\x0bstart_build\x18\x04 \x01(\x0b\x32\x1a.spex.IShipyard.StartBuildH\x00\x12\x16\n\x0c\x63\x61ncel_build\x18\x05 \x01(\x08H\x00\x12\x36\n\rspecification\x18\x14 \x01(\x0b\x32\x1d.spex.IShipyard.SpecificationH\x00\x12\x36\n\x14\x62ind_to_cargo_status\x18\x15 \x01(\x0e\x32\x16.spex.IShipyard.StatusH\x00\x12\x39\n\x0f\x62uilding_report\x18\x16 \x01(\x0b\x32\x1e.spex.IShipyard.BuildingReportH\x00\x12\x36\n\x11\x62uilding_complete\x18\x17 \x01(\x0b\x32\x19.spex.IShipyard.ShipBuiltH\x00\x1a&\n\rSpecification\x12\x15\n\rlabor_per_sec\x18\x01 \x01(\x01\x1a\x37\n\nStartBuild\x12\x16\n\x0e\x62lueprint_name\x18\x01 \x01(\t\x12\x11\n\tship_name\x18\x02 \x01(\t\x1a/\n\tShipBuilt\x12\x11\n\tship_name\x18\x01 \x01(\t\x12\x0f\n\x07slot_id\x18\x02 \x01(\r\x1aJ\n\x0e\x42uildingReport\x12&\n\x06status\x18\x01 \x01(\x0e\x32\x16.spex.IShipyard.Status\x12\x10\n\x08progress\x18\x02 \x01(\x01\"\xe3\x01\n\x06Status\x12\x0b\n\x07SUCCESS\x10\x00\x12\x12\n\x0eINTERNAL_ERROR\x10\x01\x12\x13\n\x0f\x43\x41RGO_NOT_FOUND\x10\x02\x12\x14\n\x10SHIPYARD_IS_BUSY\x10\x03\x12\x11\n\rBUILD_STARTED\x10\x04\x12\x15\n\x11\x42UILD_IN_PROGRESS\x10\x05\x12\x12\n\x0e\x42UILD_COMPLETE\x10\x06\x12\x12\n\x0e\x42UILD_CANCELED\x10\x07\x12\x10\n\x0c\x42UILD_FROZEN\x10\x08\x12\x10\n\x0c\x42UILD_FAILED\x10\t\x12\x17\n\x13\x42LUEPRINT_NOT_FOUND\x10\nB\x08\n\x06\x63hoice\"\xb1\x06\n\x0bICommutator\x12\x19\n\x0ftotal_slots_req\x18\x01 \x01(\x08H\x00\x12\x19\n\x0fmodule_info_req\x18\x02 \x01(\rH\x00\x12\x1e\n\x14\x61ll_modules_info_req\x18\x03 \x01(\x08H\x00\x12\x15\n\x0bopen_tunnel\x18\x04 \x01(\rH\x00\x12\x16\n\x0c\x63lose_tunnel\x18\x05 \x01(\rH\x00\x12\x11\n\x07monitor\x18\x06 \x01(\x08H\x00\x12\x15\n\x0btotal_slots\x18\x15 \x01(\rH\x00\x12\x33\n\x0bmodule_info\x18\x16 \x01(\x0b\x32\x1c.spex.ICommutator.ModuleInfoH\x00\x12\x1c\n\x12open_tunnel_report\x18\x17 \x01(\rH\x00\x12\x36\n\x12open_tunnel_failed\x18\x18 \x01(\x0e\x32\x18.spex.ICommutator.StatusH\x00\x12\x37\n\x13\x63lose_tunnel_status\x18\x19 \x01(\x0e\x32\x18.spex.ICommutator.StatusH\x00\x12/\n\x0bmonitor_ack\x18\x1a \x01(\x0e\x32\x18.spex.ICommutator.StatusH\x00\x12*\n\x06update\x18\x1b \x01(\x0b\x32\x18.spex.ICommutator.UpdateH\x00\x1aG\n\nModuleInfo\x12\x0f\n\x07slot_id\x18\x01 \x01(\r\x12\x13\n\x0bmodule_type\x18\x02 \x01(\t\x12\x13\n\x0bmodule_name\x18\x03 \x01(\t\x1a\x66\n\x06Update\x12\x37\n\x0fmodule_attached\x18\x01 \x01(\x0b\x32\x1c.spex.ICommutator.ModuleInfoH\x00\x12\x19\n\x0fmodule_detached\x18\x02 \x01(\rH\x00\x42\x08\n\x06\x63hoice\"\x96\x01\n\x06Status\x12\x0b\n\x07SUCCESS\x10\x00\x12\x10\n\x0cINVALID_SLOT\x10\x01\x12\x12\n\x0eMODULE_OFFLINE\x10\x02\x12\x16\n\x12REJECTED_BY_MODULE\x10\x03\x12\x12\n\x0eINVALID_TUNNEL\x10\x04\x12\x16\n\x12\x43OMMUTATOR_OFFLINE\x10\x05\x12\x15\n\x11TOO_MANY_SESSIONS\x10\x06\x42\x08\n\x06\x63hoice\"\x9b\x01\n\x05IGame\x12\x30\n\x10game_over_report\x18\x15 \x01(\x0b\x32\x14.spex.IGame.GameOverH\x00\x1a&\n\x05Score\x12\x0e\n\x06player\x18\x01 \x01(\t\x12\r\n\x05score\x18\x02 \x01(\r\x1a.\n\x08GameOver\x12\"\n\x07leaders\x18\x01 \x03(\x0b\x32\x11.spex.IGame.ScoreB\x08\n\x06\x63hoice\"\x89\x01\n\x0cISystemClock\x12\x12\n\x08time_req\x18\x01 \x01(\x08H\x00\x12\x14\n\nwait_until\x18\x02 \x01(\x04H\x00\x12\x12\n\x08wait_for\x18\x03 \x01(\x04H\x00\x12\x11\n\x07monitor\x18\x04 \x01(\rH\x00\x12\x0e\n\x04time\x18\x15 \x01(\x04H\x00\x12\x0e\n\x04ring\x18\x16 \x01(\x04H\x00\x42\x08\n\x06\x63hoice\"\xf3\x05\n\x07Message\x12\x10\n\x08tunnelId\x18\x01 \x01(\r\x12\x11\n\ttimestamp\x18\x02 \x01(\x04\x12(\n\x07session\x18\n \x01(\x0b\x32\x15.spex.ISessionControlH\x00\x12*\n\x0croot_session\x18\x0b \x01(\x0b\x32\x12.spex.IRootSessionH\x00\x12)\n\x0b\x61\x63\x63\x65ssPanel\x18\r \x01(\x0b\x32\x12.spex.IAccessPanelH\x00\x12\'\n\ncommutator\x18\x0e \x01(\x0b\x32\x11.spex.ICommutatorH\x00\x12\x1b\n\x04ship\x18\x0f \x01(\x0b\x32\x0b.spex.IShipH\x00\x12\'\n\nnavigation\x18\x10 \x01(\x0b\x32\x11.spex.INavigationH\x00\x12\x1f\n\x06\x65ngine\x18\x11 \x01(\x0b\x32\r.spex.IEngineH\x00\x12\x34\n\x11\x63\x65lestial_scanner\x18\x12 \x01(\x0b\x32\x17.spex.ICelestialScannerH\x00\x12\x30\n\x0fpassive_scanner\x18\x13 \x01(\x0b\x32\x15.spex.IPassiveScannerH\x00\x12\x32\n\x10\x61steroid_scanner\x18\x14 \x01(\x0b\x32\x16.spex.IAsteroidScannerH\x00\x12\x36\n\x12resource_container\x18\x15 \x01(\x0b\x32\x18.spex.IResourceContainerH\x00\x12.\n\x0e\x61steroid_miner\x18\x16 \x01(\x0b\x32\x14.spex.IAsteroidMinerH\x00\x12\x36\n\x12\x62lueprints_library\x18\x17 \x01(\x0b\x32\x18.spex.IBlueprintsLibraryH\x00\x12#\n\x08shipyard\x18\x18 \x01(\x0b\x32\x0f.spex.IShipyardH\x00\x12\x1b\n\x04game\x18\x19 \x01(\x0b\x32\x0b.spex.IGameH\x00\x12*\n\x0csystem_clock\x18\x1a \x01(\x0b\x32\x12.spex.ISystemClockH\x00\x42\x08\n\x06\x63hoiceB\x03\xf8\x01\x01\x62\x06proto3')

_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, globals())
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'Protocol_pb2', globals())
if _descriptor._USE_C_DESCRIPTORS == False:

  DESCRIPTOR._options = None
  DESCRIPTOR._serialized_options = b'\370\001\001'
  _ISESSIONCONTROL._serialized_start=43
  _ISESSIONCONTROL._serialized_end=130
  _IROOTSESSION._serialized_start=132
  _IROOTSESSION._serialized_end=220
  _IACCESSPANEL._serialized_start=223
  _IACCESSPANEL._serialized_end=484
  _IACCESSPANEL_LOGINREQUEST._serialized_start=376
  _IACCESSPANEL_LOGINREQUEST._serialized_end=423
  _IACCESSPANEL_ACCESSGRANTED._serialized_start=425
  _IACCESSPANEL_ACCESSGRANTED._serialized_end=474
  _IENGINE._serialized_start=487
  _IENGINE._serialized_end=878
  _IENGINE_SPECIFICATION._serialized_start=703
  _IENGINE_SPECIFICATION._serialized_end=738
  _IENGINE_CHANGETHRUST._serialized_start=740
  _IENGINE_CHANGETHRUST._serialized_end=813
  _IENGINE_CURRENTTHRUST._serialized_start=815
  _IENGINE_CURRENTTHRUST._serialized_end=868
  _ISHIP._serialized_start=881
  _ISHIP._serialized_end=1055
  _ISHIP_STATE._serialized_start=966
  _ISHIP_STATE._serialized_end=1045
  _INAVIGATION._serialized_start=1057
  _INAVIGATION._serialized_end=1140
  _ICELESTIALSCANNER._serialized_start=1143
  _ICELESTIALSCANNER._serialized_end=1776
  _ICELESTIALSCANNER_SPECIFICATION._serialized_start=1426
  _ICELESTIALSCANNER_SPECIFICATION._serialized_end=1492
  _ICELESTIALSCANNER_SCAN._serialized_start=1494
  _ICELESTIALSCANNER_SCAN._serialized_end=1554
  _ICELESTIALSCANNER_ASTEROIDINFO._serialized_start=1556
  _ICELESTIALSCANNER_ASTEROIDINFO._serialized_end=1639
  _ICELESTIALSCANNER_SCANRESULTS._serialized_start=1641
  _ICELESTIALSCANNER_SCANRESULTS._serialized_end=1725
  _ICELESTIALSCANNER_STATUS._serialized_start=1727
  _ICELESTIALSCANNER_STATUS._serialized_end=1766
  _IPASSIVESCANNER._serialized_start=1779
  _IPASSIVESCANNER._serialized_end=2107
  _IPASSIVESCANNER_SPECIFICATION._serialized_start=1979
  _IPASSIVESCANNER_SPECIFICATION._serialized_end=2050
  _IPASSIVESCANNER_UPDATE._serialized_start=2052
  _IPASSIVESCANNER_UPDATE._serialized_end=2097
  _IASTEROIDSCANNER._serialized_start=2110
  _IASTEROIDSCANNER._serialized_end=2632
  _IASTEROIDSCANNER_SPECIFICATION._serialized_start=2369
  _IASTEROIDSCANNER_SPECIFICATION._serialized_end=2432
  _IASTEROIDSCANNER_SCANRESULT._serialized_start=2434
  _IASTEROIDSCANNER_SCANRESULT._serialized_end=2555
  _IASTEROIDSCANNER_STATUS._serialized_start=2557
  _IASTEROIDSCANNER_STATUS._serialized_end=2622
  _IRESOURCECONTAINER._serialized_start=2635
  _IRESOURCECONTAINER._serialized_end=3601
  _IRESOURCECONTAINER_CONTENT._serialized_start=3165
  _IRESOURCECONTAINER_CONTENT._serialized_end=3243
  _IRESOURCECONTAINER_TRANSFER._serialized_start=3245
  _IRESOURCECONTAINER_TRANSFER._serialized_end=3330
  _IRESOURCECONTAINER_STATUS._serialized_start=3333
  _IRESOURCECONTAINER_STATUS._serialized_end=3591
  _IASTEROIDMINER._serialized_start=3604
  _IASTEROIDMINER._serialized_end=4363
  _IASTEROIDMINER_SPECIFICATION._serialized_start=4064
  _IASTEROIDMINER_SPECIFICATION._serialized_end=4149
  _IASTEROIDMINER_STATUS._serialized_start=4152
  _IASTEROIDMINER_STATUS._serialized_end=4353
  _IBLUEPRINTSLIBRARY._serialized_start=4366
  _IBLUEPRINTSLIBRARY._serialized_end=4661
  _IBLUEPRINTSLIBRARY_STATUS._serialized_start=4585
  _IBLUEPRINTSLIBRARY_STATUS._serialized_end=4651
  _ISHIPYARD._serialized_start=4664
  _ISHIPYARD._serialized_end=5493
  _ISHIPYARD_SPECIFICATION._serialized_start=5033
  _ISHIPYARD_SPECIFICATION._serialized_end=5071
  _ISHIPYARD_STARTBUILD._serialized_start=5073
  _ISHIPYARD_STARTBUILD._serialized_end=5128
  _ISHIPYARD_SHIPBUILT._serialized_start=5130
  _ISHIPYARD_SHIPBUILT._serialized_end=5177
  _ISHIPYARD_BUILDINGREPORT._serialized_start=5179
  _ISHIPYARD_BUILDINGREPORT._serialized_end=5253
  _ISHIPYARD_STATUS._serialized_start=5256
  _ISHIPYARD_STATUS._serialized_end=5483
  _ICOMMUTATOR._serialized_start=5496
  _ICOMMUTATOR._serialized_end=6313
  _ICOMMUTATOR_MODULEINFO._serialized_start=5975
  _ICOMMUTATOR_MODULEINFO._serialized_end=6046
  _ICOMMUTATOR_UPDATE._serialized_start=6048
  _ICOMMUTATOR_UPDATE._serialized_end=6150
  _ICOMMUTATOR_STATUS._serialized_start=6153
  _ICOMMUTATOR_STATUS._serialized_end=6303
  _IGAME._serialized_start=6316
  _IGAME._serialized_end=6471
  _IGAME_SCORE._serialized_start=6375
  _IGAME_SCORE._serialized_end=6413
  _IGAME_GAMEOVER._serialized_start=6415
  _IGAME_GAMEOVER._serialized_end=6461
  _ISYSTEMCLOCK._serialized_start=6474
  _ISYSTEMCLOCK._serialized_end=6611
  _MESSAGE._serialized_start=6614
  _MESSAGE._serialized_end=7369
# @@protoc_insertion_point(module_scope)
