using UnrealBuildTool;

public class RefGenEditorTarget : TargetRules
{
	public RefGenEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		SetLatest("DefaultBuildSettings");
		SetLatest("IncludeOrderVersion");
		ExtraModuleNames.AddRange(new string[] { "RefGen", "RefGenEditor" });

		// Windows: REFGEN_MSVC pins the MSVC toolset (e.g. 14.16.27023) when the engine's UBT would pick a newer
		// compiler than it supports
		string Msvc = System.Environment.GetEnvironmentVariable("REFGEN_MSVC");
		if (!string.IsNullOrEmpty(Msvc))
		{
			object Windows = GetMember(this, "WindowsPlatform");
			if (Windows != null && SetMember(Windows, "CompilerVersion", Msvc))
				System.Console.WriteLine("RefGen: pinned MSVC " + Msvc);
		}
	}

	static object GetMember(object Obj, string Name)
	{
		System.Reflection.BindingFlags Flags = System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public;
		System.Reflection.PropertyInfo Prop = Obj.GetType().GetProperty(Name, Flags);
		if (Prop != null)
			return Prop.GetValue(Obj, null);
		System.Reflection.FieldInfo Field = Obj.GetType().GetField(Name, Flags);
		return Field != null ? Field.GetValue(Obj) : null;
	}

	static bool SetMember(object Obj, string Name, object Value)
	{
		System.Reflection.BindingFlags Flags = System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public;
		System.Reflection.PropertyInfo Prop = Obj.GetType().GetProperty(Name, Flags);
		if (Prop != null && Prop.CanWrite)
		{
			Prop.SetValue(Obj, Value, null);
			return true;
		}
		System.Reflection.FieldInfo Field = Obj.GetType().GetField(Name, Flags);
		if (Field == null)
			return false;
		Field.SetValue(Obj, Value);
		return true;
	}

	// DefaultBuildSettings (4.24+) and IncludeOrderVersion (5.1+) don't exist on every engine: set them by
	// reflection so one target file builds against UE 4.20 through 5.9
	void SetLatest(string Name)
	{
		System.Reflection.BindingFlags Flags = System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public;
		System.Reflection.PropertyInfo Prop = typeof(TargetRules).GetProperty(Name, Flags);
		if (Prop != null && Prop.CanWrite)
		{
			Prop.SetValue(this, System.Enum.Parse(Prop.PropertyType, "Latest"), null);
			return;
		}
		System.Reflection.FieldInfo Field = typeof(TargetRules).GetField(Name, Flags);
		if (Field != null)
			Field.SetValue(this, System.Enum.Parse(Field.FieldType, "Latest"));
	}
}
