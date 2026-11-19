using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class RefGen : ModuleRules
{
	public RefGen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(OwnDirectory());
		string[] Wanted = new string[] { "Core", "CoreUObject", "Engine", "AssetRegistry", "GameplayTags", "GameplayAbilities", "GameplayTasks" };
		// GameplayTasks is 4.9+: only depend on the modules this engine has (same lookup as RefGenEditor.Build.cs)
		List<string> Existing = new List<string>();
		string Eng = Path.GetDirectoryName(typeof(ModuleRules).Assembly.Location);
		while (Eng != null && !(Path.GetFileName(Eng) == "Engine" && Directory.Exists(Path.Combine(Eng, "Source"))))
			Eng = Path.GetDirectoryName(Eng);
		foreach (string Root in new string[] { Path.Combine(Eng, "Source"), Path.Combine(Eng, "Plugins") })
			foreach (string File in Directory.GetFiles(Root, "*.Build.cs", SearchOption.AllDirectories))
				Existing.Add(Path.GetFileName(File).Replace(".Build.cs", ""));
		foreach (string Name in Wanted)
			if (Existing.Contains(Name))
				PublicDependencyModuleNames.Add(Name);
	}

	// ModuleDirectory is 4.8+: on older UBTs derive this module's directory from the .uproject on the command line
	string OwnDirectory()
	{
		System.Reflection.PropertyInfo Prop = typeof(ModuleRules).GetProperty("ModuleDirectory");
		if (Prop != null)
			return (string)Prop.GetValue(this, null);
		System.Reflection.FieldInfo Field = typeof(ModuleRules).GetField("ModuleDirectory");
		if (Field != null)
			return (string)Field.GetValue(this);
		foreach (string Arg in System.Environment.GetCommandLineArgs())
		{
			string Project = Arg.Substring(Arg.IndexOf('=') + 1).Trim('"'); // plain path or -project=<path>
			if (Project.EndsWith(".uproject", System.StringComparison.OrdinalIgnoreCase) && File.Exists(Project))
				return Path.Combine(Path.GetDirectoryName(Path.GetFullPath(Project)), "Source", "RefGen");
		}
		throw new System.Exception("RefGen.Build.cs: cannot find the module directory");
	}
}
