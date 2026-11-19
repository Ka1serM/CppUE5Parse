using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class RefGenEditor : ModuleRules
{
	public RefGenEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		string[] Wanted = new string[] {
			"Core", "CoreUObject", "Engine", "UnrealEd", "AssetTools", "AssetRegistry", "Kismet", "KismetCompiler",
			"BlueprintGraph", "AnimGraph", "AnimGraphRuntime", "GraphEditor", "MaterialEditor", "Landscape", "Foliage",
			"LevelSequence", "MovieScene", "MovieSceneTracks", "GameplayTags", "GameplayAbilities", "GameplayTasks",
			"AudioEditor", "RefGen", "MeshDescription", "StaticMeshDescription",
			"PhysicsCore", "PhysicsUtilities", "RenderCore", "RHI", "Json", "DeveloperSettings", "Analytics",
			// 5.9 moved Cascade (particle systems, particle anim notifies) out of Engine into the Cascade plugin
			"CascadeRuntime", "CascadeEditor",
			// catalog (RefGenCatalog*): plugin and optional modules, each also reported as RG_WITH_<NAME>
			"UMG", "UMGEditor", "Slate", "SlateCore", "Paper2D", "Niagara", "NiagaraEditor", "HairStrandsCore", "HairStrandsEditor",
			"GeometryCollectionEngine", "BSPUtils", "MediaAssets", "CinematicCamera"
		};
		string[] Reported = new string[] { "UMG", "UMGEditor", "Paper2D", "Niagara", "NiagaraEditor", "HairStrandsCore",
			"HairStrandsEditor", "GeometryCollectionEngine", "Foliage", "Landscape", "BSPUtils" };
		// Older engines lack some of these modules (PhysicsUtilities < 5.0, StaticMeshDescription < 4.24, Cascade*
		// < 5.9, ...): only depend on those that exist in this engine. The generator code guards their use by version.
		List<string> Existing = new List<string>(); // not HashSet: 4.22 and older compile rules without System.Core
		// EngineDirectory only exists from 4.20 on: walk up from UnrealBuildTool's own location to the Engine dir
		string Eng = Path.GetDirectoryName(typeof(ModuleRules).Assembly.Location);
		while (Eng != null && !(Path.GetFileName(Eng) == "Engine" && Directory.Exists(Path.Combine(Eng, "Source"))))
			Eng = Path.GetDirectoryName(Eng);
		foreach (string Root in new string[] { Path.Combine(Eng, "Source"), Path.Combine(Eng, "Plugins") })
			foreach (string File in Directory.GetFiles(Root, "*.Build.cs", SearchOption.AllDirectories))
				Existing.Add(Path.GetFileName(File).Replace(".Build.cs", ""));
		Existing.Add("RefGen");
		foreach (string Name in Wanted)
			if (Existing.Contains(Name))
				PrivateDependencyModuleNames.Add(Name);
		// PublicDefinitions from 4.19 on, Definitions before: found by reflection so one file builds everywhere
		System.Reflection.FieldInfo DefinitionsField = typeof(ModuleRules).GetField("PublicDefinitions") ?? typeof(ModuleRules).GetField("Definitions");
		List<string> Definitions = DefinitionsField.GetValue(this) as List<string>;
		foreach (string Name in Reported)
			Definitions.Add("RG_WITH_" + Name.ToUpperInvariant() + "=" + (Existing.Contains(Name) ? "1" : "0"));
	}
}
