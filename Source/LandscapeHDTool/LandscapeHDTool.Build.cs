using UnrealBuildTool;

public class LandscapeHDTool : ModuleRules
{
	public LandscapeHDTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Landscape",
			"Foliage",
			"Projects"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UnrealEd",
			"InputCore",
			"WorkspaceMenuStructure",
			"ToolMenus"
		});
	}
}