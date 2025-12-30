// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "VoxelStamp.h"
#include "VoxelPinValue.h"
#include "Shape/VoxelShape.h"
#include "Shape/VoxelShapeStamp.h"
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

	TArray<UScriptStruct*> AllStructs = GetDerivedStructs<FVoxelShape>(false);

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

	FString ShapeStampHeader;
	FSourceCodeNavigation::FindClassHeaderPath(FVoxelShapeStamp::StaticStruct(), ShapeStampHeader);
	if (!ShapeStampHeader.IsEmpty())
	{
		ShapeStampHeader = FPaths::GetBaseFilename(ShapeStampHeader);
	}

	FString ShapeStampName = FVoxelShapeStamp::StaticStruct()->GetName();

	bool bModified = false;
	ON_SCOPE_EXIT
	{
		if (bModified)
		{
			ensure(false);
			FPlatformMisc::RequestExit(true);
		}
	};

	const auto ForeachShapeStampProperty = [&](const bool bIncludeParents, auto Lambda)
	{
		for (const FProperty& Property : GetStructProperties(FVoxelShapeStamp::StaticStruct(), bIncludeParents ? EFieldIterationFlags::IncludeSuper : EFieldIterationFlags::None))
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

		FString LibraryFile;
		LibraryFile += "// Copyright Voxel Plugin SAS. All Rights Reserved.\n";
		LibraryFile += "\n";
		LibraryFile += "#pragma once\n";
		LibraryFile += "\n";
		LibraryFile += "#include \"VoxelMinimal.h\"\n";
		LibraryFile += "#include \"" + HeaderName + ".h\"\n";
		if (!ShapeStampHeader.IsEmpty())
		{
			LibraryFile += "#include \"" + ShapeStampHeader + "Ref.h\"\n";
		}
		LibraryFile += "#include \"VoxelStampBlueprintFunctionLibrary.h\"\n";
		LibraryFile += "#include \"" + HeaderName + "_K2.generated.h\"\n";
		LibraryFile += "\n";
		LibraryFile += "////////////////////////////////////////////////////\n";
		LibraryFile += "///////// The code below is auto-generated /////////\n";
		LibraryFile += "////////////////////////////////////////////////////\n";
		LibraryFile += "\n";

		FVoxelShapeStamp ShapeStamp;

		for (UScriptStruct* Struct : Structs)
		{
			const FString Name = Struct->GetName();
			const FString DisplayName = Struct->GetDisplayNameText().ToString();

			FString NameWithoutVoxel = Name;
			NameWithoutVoxel.RemoveFromStart("Voxel");

			const FString DisplayNameWithoutVoxel = FName::NameToDisplayString(NameWithoutVoxel, false);

			const FString Api = Struct->GetOutermost()->GetName().Replace(TEXT("/Script/"), TEXT("")).ToUpper() + "_API";
			const FString Category = "Voxel|Stamp|Shape|" + DisplayNameWithoutVoxel.Replace(TEXT(" Shape"), TEXT(""));

			const TSharedRef<const FVoxelShape> Template = MakeSharedStruct<FVoxelShape>(Struct);

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

			LibraryFile += "UCLASS()\n";
			LibraryFile += "class " + Api + " U" + Name + "_K2 : public UVoxelStampBlueprintFunctionLibrary\n";
			LibraryFile += "{\n";
			LibraryFile += "\tGENERATED_BODY()\n";
			LibraryFile += "\n";
			LibraryFile += "public:\n";

			LibraryFile += "\tUFUNCTION(BlueprintCallable, Category = \"" + Category + "\", DisplayName = \"Make " + DisplayName + " Stamp\", meta = (Keywords = \"Construct, Create\"";

			ForeachShapeStampProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				const FVoxelPinValue Value = FVoxelPinValue::MakeFromProperty(
					Property,
					Property.ContainerPtrToValuePtr<void>(&ShapeStamp));

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

			ForeachShapeStampProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				if (Property.IsA<FArrayProperty>() ||
					Property.IsA<FMapProperty>() ||
					Property.IsA<FSetProperty>())
				{
					LibraryFile += PropertyName + ", ";
				}
			});

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
			LibraryFile += "\t\tF" + ShapeStampName + "Ref& Stamp";

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

			ForeachShapeStampProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
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
			LibraryFile += "\t\tStamp = F" + ShapeStampName + "Ref::New();\n";

			ForeachShapeStampProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				LibraryFile += "\t\tStamp->" + PropertyName + " = " + PropertyName + ";\n";
			});

			{
				FProperty& ShapeProperty = FindFPropertyChecked(FVoxelShapeStamp, Shape);

				LibraryFile += "\t\tTVoxelInstancedStruct<F" + Name + "> Shape(F" + Name + "::StaticStruct());\n";
				
				ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
				{
					LibraryFile += "\t\tShape->" + PropertyName + " = " + PropertyName + ";\n";
				});

				LibraryFile += "\t\tStamp->" + ShapeProperty.GetNameCPP() + " = Shape;\n";
			}
			LibraryFile += "\t}\n";
			LibraryFile += "\n";
			LibraryFile += "\tUFUNCTION(BlueprintPure, Category = \"" + Category + "\", DisplayName = \"Break " + DisplayName + " Stamp\", meta = (Keywords = \"Break\"))\n";
			LibraryFile += "\tstatic void Break(\n";
			LibraryFile += "\t\tconst F" + ShapeStampName + "Ref Stamp";

			ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				LibraryFile += ",\n\t\tUPARAM(meta = (DisplayName = \"" +
					Property.GetDisplayNameText().ToString() + "\", ToolTip = \"" +
					Property.GetToolTipText().ToString().ReplaceCharWithEscapedChar() + "\")) " + Type + "& " + PropertyName;
			});

			ForeachShapeStampProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				LibraryFile += ",\n\t\tUPARAM(meta = (DisplayName = \"" +
					Property.GetDisplayNameText().ToString() + "\", ToolTip = \"" +
					Property.GetToolTipText().ToString().ReplaceCharWithEscapedChar() + "\")) " + Type + "& " + PropertyName;
			});

			LibraryFile += ")\n";
			LibraryFile += "\t{\n";
			ForeachShapeStampProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				LibraryFile += "\t\t" + PropertyName + " = FVoxelUtilities::MakeSafe<" + Type + ">();\n";
			});
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
			LibraryFile += "\t\tF" + Name + "* Shape = Stamp->Shape->As<F" + Name + ">();\n";
			LibraryFile += "\t\tif (!Shape)\n";
			LibraryFile += "\t\t{\n";
			LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
			LibraryFile += "\t\t\treturn;\n";
			LibraryFile += "\t\t}\n";
			LibraryFile += "\n";

			ForeachShapeStampProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				LibraryFile += "\t\t" + PropertyName + " = Stamp->" + PropertyName + ";\n";
			});

			ForeachProperty(true, [&](const FProperty& Property, const FString& PropertyName, const FString& Type)
			{
				LibraryFile += "\t\t" + PropertyName + " = Shape->" + PropertyName + ";\n";
			});

			LibraryFile += "\t}\n";
			LibraryFile += "\n";

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
				LibraryFile += "\tstatic void Get" + FunctionName + "(UPARAM(Required) F" + ShapeStampName + "Ref Stamp, " + Type + "& " + PropertyName + ")\n";
				LibraryFile += "\t{\n";
				LibraryFile += "\t\t" + PropertyName + " = FVoxelUtilities::MakeSafe<" + Type + ">();\n";
				LibraryFile += "\n";
				LibraryFile += "\t\tif (!Stamp.IsValid())\n";
				LibraryFile += "\t\t{\n";
				LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
				LibraryFile += "\t\t\treturn;\n";
				LibraryFile += "\t\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\t\tF" + Name + "* Shape = Stamp->Shape->As<F" + Name + ">();\n";
				LibraryFile += "\t\tif (!Shape)\n";
				LibraryFile += "\t\t{\n";
				LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
				LibraryFile += "\t\t\treturn;\n";
				LibraryFile += "\t\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\t\t" + PropertyName + " = Shape->" + PropertyName + ";\n";
				LibraryFile += "\t}\n";
				LibraryFile += "\n";

				LibraryFile += "\t// Set " + Property.GetDisplayNameText().ToString() + "\n";
				LibraryFile += "\t// This will automatically refresh the stamp\n";
				LibraryFile += Tooltip;
				LibraryFile += "\tUFUNCTION(BlueprintCallable, Category = \"" + Category + "\", DisplayName = \"Set " + Property.GetDisplayNameText().ToString() + "\")\n";
				LibraryFile += "\tstatic void Set" + FunctionName + "(UPARAM(Required) F" + ShapeStampName + "Ref Stamp, UPARAM(meta = (DisplayName = \"Stamp\")) F" + ShapeStampName + "Ref& OutStamp, " + Type + " " + PropertyName + ")\n";
				LibraryFile += "\t{\n";
				LibraryFile += "\t\tOutStamp = Stamp;\n";
				LibraryFile += "\t\t\n";
				LibraryFile += "\t\tif (!Stamp.IsValid())\n";
				LibraryFile += "\t\t{\n";
				LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
				LibraryFile += "\t\t\treturn;\n";
				LibraryFile += "\t\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\t\tF" + Name + "* Shape = Stamp->Shape->As<F" + Name + ">();\n";
				LibraryFile += "\t\tif (!Shape)\n";
				LibraryFile += "\t\t{\n";
				LibraryFile += "\t\t\tVOXEL_MESSAGE(Error, \"Stamp is invalid\");\n";
				LibraryFile += "\t\t\treturn;\n";
				LibraryFile += "\t\t}\n";
				LibraryFile += "\n";
				LibraryFile += "\t\tShape->" + PropertyName + " = " + PropertyName + ";\n";
				LibraryFile += "\t\tStamp.Update();\n";
				LibraryFile += "\t}\n";
				LibraryFile += "\n";
			});

			LibraryFile.RemoveFromEnd("\n");
			LibraryFile += "};\n";
			LibraryFile += "\n";
		}

		LibraryFile.RemoveFromEnd("\n");
		LibraryFile.RemoveFromEnd("\n");

		const FString BasePath = FPaths::GetPath(FPaths::ConvertRelativePathToFull(Header));

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
#endif