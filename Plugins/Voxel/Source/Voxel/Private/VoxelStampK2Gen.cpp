// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "VoxelStamp.h"
#include "VoxelPinValue.h"
#if WITH_EDITOR
#include "SourceCodeNavigation.h"
#endif

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME()
{
	if (!FVoxelUtilities::IsDevWorkflow())
	{
		return;
	}

	TArray<UScriptStruct*> AllStructs = GetDerivedStructs<FVoxelStamp>(true);

	AllStructs.RemoveAll([](const UScriptStruct* Struct)
	{
		return Struct->HasMetaDataHierarchical("Internal");
	});

	AllStructs.Sort([](const UScriptStruct& A, const UScriptStruct& B)
	{
		return A.GetName() < B.GetName();
	});

	TVoxelMap<FString, TArray<UScriptStruct*>> HeaderToStructs;
	TVoxelMap<UScriptStruct*, FString> StructToHeader;

	for (UScriptStruct* Struct : AllStructs)
	{
		FString Header;
		if (!FSourceCodeNavigation::FindClassHeaderPath(Struct, Header))
		{
			continue;
		}

		HeaderToStructs.FindOrAdd(Header).Add(Struct);
		StructToHeader.Add_EnsureNew(Struct, Header);
	}

	bool bModified = false;
	ON_SCOPE_EXIT
	{
		if (bModified)
		{
			ensure(false);
			FPlatformMisc::RequestExit(true);
		}
	};

	for (const auto& It : HeaderToStructs)
	{
		const FString Header = It.Key;
		const FString HeaderName = FPaths::GetBaseFilename(Header);
		const TArray<UScriptStruct*>& Structs = It.Value;

		if (Structs.Contains(FVoxelStamp::StaticStruct()))
		{
			check(Structs.Num() == 1);
			continue;
		}

		FString StampRefFile;
		StampRefFile += "// Copyright Voxel Plugin SAS. All Rights Reserved.\n";
		StampRefFile += "\n";
		StampRefFile += "#pragma once\n";
		StampRefFile += "\n";
		StampRefFile += "#include \"VoxelMinimal.h\"\n";
		for (UScriptStruct* Struct : Structs)
		{
			FString ParentPath = StructToHeader[CastChecked<UScriptStruct>(Struct->GetSuperStruct())];

			const int32 Index = ParentPath.Find(TEXT("/Public/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			check(Index != -1);

			ParentPath = ParentPath.RightChop(Index + 8);
			verify(ParentPath.RemoveFromEnd(".h"));

			StampRefFile += "#include \"" + ParentPath + "Ref.h\"\n";
		}
		StampRefFile += "#include \"" + HeaderName + ".h\"\n";
		StampRefFile += "#include \"" + HeaderName + "Ref.generated.h\"\n";
		StampRefFile += "\n";
		StampRefFile += "////////////////////////////////////////////////////\n";
		StampRefFile += "///////// The code below is auto-generated /////////\n";
		StampRefFile += "////////////////////////////////////////////////////\n";
		StampRefFile += "\n";

		FString LibraryFile;
		LibraryFile += "// Copyright Voxel Plugin SAS. All Rights Reserved.\n";
		LibraryFile += "\n";
		LibraryFile += "#pragma once\n";
		LibraryFile += "\n";
		LibraryFile += "#include \"VoxelMinimal.h\"\n";
		LibraryFile += "#include \"" + HeaderName + "Ref.h\"\n";
		LibraryFile += "#include \"VoxelStampBlueprintFunctionLibrary.h\"\n";
		LibraryFile += "#include \"" + HeaderName + "_K2.generated.h\"\n";
		LibraryFile += "\n";
		LibraryFile += "////////////////////////////////////////////////////\n";
		LibraryFile += "///////// The code below is auto-generated /////////\n";
		LibraryFile += "////////////////////////////////////////////////////\n";
		LibraryFile += "\n";

		for (UScriptStruct* Struct : Structs)
		{
			const FString Name = Struct->GetName();
			const FString DisplayName = Struct->GetDisplayNameText().ToString();

			FString NameWithoutVoxel = Name;
			NameWithoutVoxel.RemoveFromStart("Voxel");

			const FString DisplayNameWithoutVoxel = FName::NameToDisplayString(NameWithoutVoxel, false);

			const FString Api = Struct->GetOutermost()->GetName().Replace(TEXT("/Script/"), TEXT("")).ToUpper() + "_API";
			const FString Category = "Voxel|Stamp|" + DisplayNameWithoutVoxel.Replace(TEXT(" Stamp"), TEXT(""));

			const TSharedRef<const FVoxelStamp> Template = MakeSharedStruct<FVoxelStamp>(Struct);

			const auto ForeachProperty = [&](const bool bIncludeParents, auto Lambda)
			{
				for (const FProperty& Property : GetStructProperties(Struct, bIncludeParents ? EFieldIterationFlags::IncludeSuper : EFieldIterationFlags::None))
				{
					if (!Property.HasAllPropertyFlags(CPF_Edit) ||
						Property.HasMetaData("NoK2"))
					{
						continue;
					}

					if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
					{
						if (!StructProperty->Struct->GetBoolMetaDataHierarchical("BlueprintType"))
						{
							continue;
						}
					}

					FString PropertyName = Property.GetNameCPP();
					if (PropertyName == "VoxelLayers")
					{
						PropertyName = "Layers";
					}

					Lambda(Property, PropertyName, FVoxelUtilities::GetFunctionType(Property));
				}
			};

			const bool bIsFinal = Struct->GetSuperStruct() != FVoxelStamp::StaticStruct();

			StampRefFile += "USTRUCT(BlueprintType, DisplayName = \"" + DisplayName + "\", Category = \"" + Category + "\", meta = (";
			StampRefFile += "HasNativeMake = \"" + Struct->GetOutermost()->GetName() + "." + Name + "_K2.Make\", ";
			StampRefFile += "HasNativeBreak = \"" + Struct->GetOutermost()->GetName() + "." + Name + "_K2.Break\"))\n";
			StampRefFile += "struct " + Api + " F" + Name + "Ref " + (bIsFinal ? "final " : "") + ": public F" + Struct->GetSuperStruct()->GetName() + "Ref\n";
			StampRefFile += "{\n";
			StampRefFile += "\tGENERATED_BODY()\n";
			StampRefFile += "\t" + FString(bIsFinal ? "GENERATED_VOXEL_STAMP_REF_BODY" : "GENERATED_VOXEL_STAMP_REF_PARENT_BODY") + "(F" + Name + ")\n";
			StampRefFile += "};\n";
			StampRefFile += "\n";

			StampRefFile += "template<>\n";
			StampRefFile += "struct TStructOpsTypeTraits<F" + Name + "Ref> : TStructOpsTypeTraits<FVoxelStampRef>\n";
			StampRefFile += "{\n";
			StampRefFile += "};\n";
			StampRefFile += "\n";

			StampRefFile += "template<>\n";
			StampRefFile += "struct TVoxelStampRefImpl<F" + Name + ">\n";
			StampRefFile += "{\n";
			StampRefFile += "\tusing Type = F" + Name + "Ref;\n";
			StampRefFile += "};\n";

			LibraryFile += "UCLASS()\n";
			LibraryFile += "class " + Api + " U" + Name + "_K2 : public UVoxelStampBlueprintFunctionLibrary\n";
			LibraryFile += "{\n";
			LibraryFile += "\tGENERATED_BODY()\n";
			LibraryFile += "\n";
			LibraryFile += "public:\n";

			LibraryFile += "\tUFUNCTION(BlueprintCallable, Category = \"Voxel|Stamp|Casting\", meta = (ExpandEnumAsExecs = \"Result\"))\n";
			LibraryFile += "\tstatic F" + Name + "Ref CastTo" + NameWithoutVoxel + "(const FVoxelStampRef Stamp, EVoxelStampCastResult& Result)\n";
			LibraryFile += "\t{\n";
			LibraryFile += "\t\treturn CastToStampImpl<F" + Name + ">(Stamp, Result);\n";
			LibraryFile += "\t}\n";
			LibraryFile += "\n";

			LibraryFile += "\t// Make a copy of this stamp\n";
			LibraryFile += "\t// You can then call Set XXXX on the copy without having the original stamp be modified\n";
			LibraryFile += "\tUFUNCTION(BlueprintPure, Category = \"" + Category + "\", DisplayName = \"Get " + DisplayNameWithoutVoxel + "\")\n";
			LibraryFile += "\tstatic void MakeCopy(const F" + Name + "Ref Stamp, F" + Name + "Ref& Copy)\n";
			LibraryFile += "\t{\n";
			LibraryFile += "\t\tCopy = F" + Name + "Ref(Stamp.MakeCopy());\n";
			LibraryFile += "\t}\n";
			LibraryFile += "\n";

			if (Struct->HasMetaData("Abstract"))
			{
				LibraryFile += "\tUFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)\n";
				LibraryFile += "\tstatic void Make(F" + Name + "Ref& Stamp)\n";
				LibraryFile += "\t{\n";
				LibraryFile += "\t\tStamp = {};\n";
				LibraryFile += "\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\tUFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)\n";
				LibraryFile += "\tstatic void Break(const F" + Name + "Ref Stamp)\n";
				LibraryFile += "\t{\n";
				LibraryFile += "\t}\n";
				LibraryFile += "\n";
			}
			else if (!Struct->HasMetaData("NoK2Make"))
			{
				LibraryFile += "\tUFUNCTION(BlueprintCallable, Category = \"" + Category + "\", DisplayName = \"Make " + DisplayName + "\", meta = (Keywords = \"Construct, Create\"";

				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
				{
					const FVoxelPinValue Value = FVoxelPinValue::MakeFromProperty(
						Property,
						Property.ContainerPtrToValuePtr<void>(&Template.Get()));

					if (Value.IsArray())
					{
						return;
					}

					const FString Default = Value.ExportToString();
					if (Default.IsEmpty())
					{
						return;
					}

					LibraryFile += ", " + PropertyName + " = \"" + Default.ReplaceCharWithEscapedChar() + "\"";
				});

				LibraryFile += ", AutoCreateRefTerm = \"";

				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
				{
					if (Property.IsA<FArrayProperty>() ||
						Property.IsA<FMapProperty>() ||
						Property.IsA<FSetProperty>())
					{
						LibraryFile += PropertyName + ", ";
					}
				});

				LibraryFile.RemoveFromEnd(", ");

				LibraryFile += "\"))\n";
				LibraryFile += "\tstatic void Make(\n";
				LibraryFile += "\t\tF" + Name + "Ref& Stamp";

				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, FString Type)
				{
					FString AllowedClasses;
					if (const FString* AllowedClassesPtr = Property.FindMetaData("AllowedClasses"))
					{
						AllowedClasses = ", AllowedClasses = \"" + *AllowedClassesPtr + "\"";
					}

					LibraryFile += ",\n\t\tUPARAM(meta = (DisplayName = \"" +
						Property.GetDisplayNameText().ToString() + "\", ToolTip = \"" +
						Property.GetToolTipText().ToString().ReplaceCharWithEscapedChar() + "\"" + AllowedClasses + ")) " + Type + " " + PropertyName;
				});

				LibraryFile += ")\n";
				LibraryFile += "\t{\n";
				LibraryFile += "\t\tStamp = F" + Name + "Ref::New();\n";

				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
				{
					LibraryFile += "\t\tStamp->" + PropertyName + " = " + PropertyName + ";\n";
				});

				LibraryFile += "\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\tUFUNCTION(BlueprintPure, Category = \"" + Category + "\", DisplayName = \"Break " + DisplayName + "\", meta = (Keywords = \"Break\"))\n";
				LibraryFile += "\tstatic void Break(\n";
				LibraryFile += "\t\tconst F" + Name + "Ref Stamp";

				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
				{
					LibraryFile += ",\n\t\tUPARAM(meta = (DisplayName = \"" +
						Property.GetDisplayNameText().ToString() + "\", ToolTip = \"" +
						Property.GetToolTipText().ToString().ReplaceCharWithEscapedChar() + "\")) " + Type + "& " + PropertyName;
				});

				LibraryFile += ")\n";
				LibraryFile += "\t{\n";
				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
				{
					LibraryFile += "\t\t" + PropertyName + " = FVoxelUtilities::MakeSafe<" + Type + ">();\n";
				});
				LibraryFile += "\n";
				LibraryFile += "\t\tif (!Stamp.IsValid())\n";
				LibraryFile += "\t\t{\n";
				LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
				LibraryFile += "\t\t\treturn;\n";
				LibraryFile += "\t\t}\n";
				LibraryFile += "\n";

				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
				{
					LibraryFile += "\t\t" + PropertyName + " = Stamp->" + PropertyName + ";\n";
				});

				LibraryFile += "\t}\n";
				LibraryFile += "\n";
			}

			ForeachProperty(false, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				FString FunctionName = PropertyName;
				if (Property.IsA<FBoolProperty>())
				{
					FunctionName.RemoveFromStart("b", ESearchCase::CaseSensitive);
				}

				FString Tooltip;
				if (!Property.GetToolTipText().IsEmpty())
				{
					Tooltip = "\n" + Property.GetToolTipText().ToString();
					Tooltip.ReplaceInline(TEXT("\n"), TEXT("\n\t// "));
					Tooltip += "\n";
					Tooltip.RemoveFromStart("\n");
					Tooltip = "\t//\n" + Tooltip;
				}

				LibraryFile += "\t// Get " + Property.GetDisplayNameText().ToString() + "\n";
				LibraryFile += Tooltip;
				LibraryFile += "\tUFUNCTION(BlueprintPure, Category = \"" + Category + "\", DisplayName = \"Get " + Property.GetDisplayNameText().ToString() + "\")\n";
				LibraryFile += "\tstatic void Get" + FunctionName + "(UPARAM(Required) F" + Name + "Ref Stamp, " + Type + "& " + PropertyName + ")\n";
				LibraryFile += "\t{\n";
				LibraryFile += "\t\t" + PropertyName + " = FVoxelUtilities::MakeSafe<" + Type + ">();\n";
				LibraryFile += "\n";
				LibraryFile += "\t\tif (!Stamp.IsValid())\n";
				LibraryFile += "\t\t{\n";
				LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
				LibraryFile += "\t\t\treturn;\n";
				LibraryFile += "\t\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\t\t" + PropertyName + " = Stamp->" + PropertyName + ";\n";
				LibraryFile += "\t}\n";
				LibraryFile += "\n";

				LibraryFile += "\t// Set " + Property.GetDisplayNameText().ToString() + "\n";
				LibraryFile += "\t// This will automatically refresh the stamp\n";
				LibraryFile += Tooltip;
				LibraryFile += "\tUFUNCTION(BlueprintCallable, Category = \"" + Category + "\", DisplayName = \"Set " + Property.GetDisplayNameText().ToString() + "\")\n";
				LibraryFile += "\tstatic void Set" + FunctionName + "(UPARAM(Required) F" + Name + "Ref Stamp, UPARAM(meta = (DisplayName = \"Stamp\")) F" + Name + "Ref& OutStamp, " + Type + " " + PropertyName + ")\n";
				LibraryFile += "\t{\n";
				LibraryFile += "\t\tOutStamp = Stamp;\n";
				LibraryFile += "\t\t\n";
				LibraryFile += "\t\tif (!Stamp.IsValid())\n";
				LibraryFile += "\t\t{\n";
				LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
				LibraryFile += "\t\t\treturn;\n";
				LibraryFile += "\t\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\t\tStamp->" + PropertyName + " = " + PropertyName + ";\n";
				LibraryFile += "\t\tStamp.Update();\n";
				LibraryFile += "\t}\n";
				LibraryFile += "\n";
			});

			LibraryFile.RemoveFromEnd("\n");
			LibraryFile += "};\n";
			LibraryFile += "\n";
		}

		StampRefFile.RemoveFromEnd("\n");
		StampRefFile.RemoveFromEnd("\n");

		LibraryFile.RemoveFromEnd("\n");
		LibraryFile.RemoveFromEnd("\n");

		const FString BasePath = FPaths::GetPath(FPaths::ConvertRelativePathToFull(Header));

		{
			const FString FilePath = BasePath / FPaths::GetBaseFilename(Header) + "Ref.h";

			FString ExistingFile;
			FFileHelper::LoadFileToString(ExistingFile, *FilePath);

			// Normalize line endings
			ExistingFile.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

			if (!ExistingFile.Equals(StampRefFile))
			{
				bModified = true;
				IFileManager::Get().Delete(*FilePath, false, true);
				ensure(FFileHelper::SaveStringToFile(StampRefFile, *FilePath));
				LOG_VOXEL(Error, "%s written", *FilePath);
			}
		}

		{
			const FString FilePath = BasePath / FPaths::GetBaseFilename(Header) + "_K2.h";

			FString ExistingFile;
			FFileHelper::LoadFileToString(ExistingFile, *FilePath);

			// Normalize line endings
			ExistingFile.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

			if (!ExistingFile.Equals(LibraryFile))
			{
				bModified = true;
				IFileManager::Get().Delete(*FilePath, false, true);
				ensure(FFileHelper::SaveStringToFile(LibraryFile, *FilePath));
				LOG_VOXEL(Error, "%s written", *FilePath);
			}
		}
	}
}
#endif