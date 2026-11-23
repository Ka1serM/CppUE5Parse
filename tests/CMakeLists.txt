file(GLOB asset_sources CONFIGURE_DEPENDS Assets/*.cpp)
add_executable(ue5parse_asset_tests ${asset_sources})
target_include_directories(ue5parse_asset_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ue5parse_asset_tests PRIVATE CppUE5Parse::Core CppUE5Parse::AnimGraphRuntime CppUE5Parse::NavigationSystem CppUE5Parse::GameplayAbilities CppUE5Parse::ACLPlugin CppUE5Parse::Paper2D CppUE5Parse::MediaAssets CppUE5Parse::ImgMedia CppUE5Parse::Landscape CppUE5Parse::Foliage CppUE5Parse::AIModule CppUE5Parse::UMG CppUE5Parse::FaceFX CppUE5Parse::CommonUI CppUE5Parse::IKRig CppUE5Parse::ControlRig CppUE5Parse::HeadMountedDisplay CppUE5Parse::AudioMixer CppUE5Parse::AudioCapture CppUE5Parse::ImgMediaEngine CppUE5Parse::AugmentedReality CppUE5Parse::XRBase CppUE5Parse::MRMesh CppUE5Parse::ProceduralMeshComponent CppUE5Parse::CustomMeshComponent CppUE5Parse::Renderer CppUE5Parse::ComputeFramework CppUE5Parse::ModelingComponents CppUE5Parse::InteractiveToolsFramework CppUE5Parse::CompositeCore CppUE5Parse::SteamVR CppUE5Parse::SteamVRInputDevice CppUE5Parse::StateTreeModule CppUE5Parse::AppleARKitFaceSupport CppUE5Parse::FacialAnimation CppUE5Parse::MagicLeap CppUE5Parse::MagicLeapController CppUE5Parse::MagicLeapARPin CppUE5Parse::MagicLeapHandMeshing CppUE5Parse::MagicLeapImageTracker CppUE5Parse::MagicLeapPlanes CppUE5Parse::GearVRController CppUE5Parse::LinearTimecode CppUE5Parse::LiveLink CppUE5Parse::OculusHMD CppUE5Parse::OculusRift CppUE5Parse::PhysXVehicles CppUE5Parse::GeometryMask CppUE5Parse::CableComponent CppUE5Parse::CinematicCamera CppUE5Parse::FunctionalTesting CppUE5Parse::GeometryCache CppUE5Parse::MediaPlate CppUE5Parse::MediaCompositing CppUE5Parse::ArchVisCharacter CppUE5Parse::VariantManagerContent CppUE5Parse::DatasmithContent CppUE5Parse::MassEntityTestSuite CppUE5Parse::LeapMotionController CppUE5Parse::SlateIMInGame CppUE5Parse::MovieScene CppUE5Parse::MovieSceneTracks CppUE5Parse::LevelSequence CppUE5Parse::Niagara CppUE5Parse::ChaosSolverEngine CppUE5Parse::FieldSystemEngine CppUE5Parse::DataflowEngine CppUE5Parse::GeometryCollectionEngine CppUE5Parse::ISMPool CppUE5Parse::ChaosCaching CppUE5Parse::DataflowEnginePlugin CppUE5Parse::HairStrandsCore CppUE5Parse::HairStrandsSolver CppUE5Parse::SmartObjectsModule CppUE5Parse::SmartObjectsTestSuite CppUE5Parse::MetasoundEngine CppUE5Parse::PCG CppUE5Parse::Text3D CppUE5Parse::GeometryFramework CppUE5Parse::NetcodeUnitTest CppUE5Parse::ActorSequence CppUE5Parse::GameTypes CppUE5Parse::Json CppUE5Parse::TextureConversion CppUE5Parse::Conversion)
ue5parse_set_library_options(ue5parse_asset_tests)

set(paks ${CMAKE_CURRENT_SOURCE_DIR}/Paks)
set(pak_folders ${paks}/Windows - ${paks}/Windows-Vulkan vulkan/)
file(GLOB pak_inputs CONFIGURE_DEPENDS ${paks}/*/*.pak ${paks}/*/*.json)
set(asset_test_file ${CMAKE_CURRENT_BINARY_DIR}/asset_tests.cmake)
add_custom_command(
    OUTPUT ${asset_test_file}
    COMMAND ue5parse_asset_tests --list ${asset_test_file} $<TARGET_FILE:ue5parse_asset_tests> ${pak_folders}
    DEPENDS ue5parse_asset_tests ${pak_inputs}
    COMMENT "Listing the asset tests"
    VERBATIM)
add_custom_target(ue5parse_asset_test_list ALL DEPENDS ${asset_test_file})
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${asset_test_file})

file(GLOB unit_sources CONFIGURE_DEPENDS Units/*.cpp)
add_executable(ue5parse_unit_tests ${unit_sources})
target_include_directories(ue5parse_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ue5parse_unit_tests PRIVATE CppUE5Parse::Core CppUE5Parse::GameTypes CppUE5Parse::MovieScene CppUE5Parse::NavigationSystem CppUE5Parse::Niagara CppUE5Parse::RigVM CppUE5Parse::ControlRig CppUE5Parse::Landscape CppUE5Parse::StateTreeModule CppUE5Parse::TextureConversion CppUE5Parse::Conversion ue5parse_aes)
ue5parse_set_library_options(ue5parse_unit_tests)
target_link_libraries(ue5parse_unit_tests PRIVATE CppUE5Parse::ComputeFramework)
add_test(NAME unit COMMAND ue5parse_unit_tests)
