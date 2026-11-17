using UnrealBuildTool;

public class RefGenTarget : TargetRules
{
	public RefGenTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		SetLatest("DefaultBuildSettings");
		SetLatest("IncludeOrderVersion");
		ExtraModuleNames.Add("RefGen");
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
