#pragma once

#include "CoreMinimal.h"
#include "PathDataLoadHelper.h"

// Editor 모듈의 시트 파서에서 공통으로 사용하는 변환 및 에셋 유틸입니다.
namespace SheetParserUtils
{
	// 셀 앞뒤의 공백을 제거합니다.
	inline FString TrimCell(const FString& Value)
	{
		return Value.TrimStartAndEnd();
	}

	// 빈 문자열과 "None"을 미설정 값으로 처리합니다.
	inline bool IsUnsetValue(const FString& Value)
	{
		const FString Trimmed = TrimCell(Value);
		return Trimmed.IsEmpty()
			|| Trimmed.Equals(TEXT("None"), ESearchCase::IgnoreCase);
	}

	namespace Detail
	{
		inline bool TryParseInt(const FString& Value, int32& OutValue)
		{
			return !IsUnsetValue(Value)
				&& LexTryParseString(OutValue, *TrimCell(Value));
		}

		inline bool TryParseFloat(const FString& Value, float& OutValue)
		{
			return !IsUnsetValue(Value)
				&& LexTryParseString(OutValue, *TrimCell(Value));
		}

		inline bool TryParseFloatArray(
			const FString& Value,
			TArray<float>& OutValues,
			const FString& Delimiter = TEXT(","))
		{
			OutValues.Reset();
			TArray<FString> Tokens;
			Value.ParseIntoArray(Tokens, *Delimiter, true);

			for (const FString& Token : Tokens)
			{
				float Parsed = 0.0f;
				if (!TryParseFloat(Token, Parsed))
				{
					OutValues.Reset();
					return false;
				}
				OutValues.Add(Parsed);
			}
			return true;
		}
	}

	// 변환할 수 없으면 DefaultValue를 반환합니다.
	inline int32 ParseIntValue(
		const FString& Value,
		const int32 DefaultValue)
	{
		int32 Parsed = DefaultValue;
		return Detail::TryParseInt(Value, Parsed) ? Parsed : DefaultValue;
	}

	// 변환할 수 없으면 DefaultValue를 반환합니다.
	inline float ParseFloatValue(
		const FString& Value,
		const float DefaultValue)
	{
		float Parsed = DefaultValue;
		return Detail::TryParseFloat(Value, Parsed) ? Parsed : DefaultValue;
	}

	// true, false, 1, 0, yes, no 형식을 지원합니다.
	inline bool ParseBoolValue(
		const FString& Value,
		const bool DefaultValue)
	{
		if (IsUnsetValue(Value))
		{
			return DefaultValue;
		}

		const FString Trimmed = TrimCell(Value);
		if (Trimmed.Equals(TEXT("true"), ESearchCase::IgnoreCase)
			|| Trimmed == TEXT("1")
			|| Trimmed.Equals(TEXT("yes"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		if (Trimmed.Equals(TEXT("false"), ESearchCase::IgnoreCase)
			|| Trimmed == TEXT("0")
			|| Trimmed.Equals(TEXT("no"), ESearchCase::IgnoreCase))
		{
			return false;
		}
		return DefaultValue;
	}

	// Enum 이름을 값으로 변환하고 실패하면 DefaultValue를 반환합니다.
	template <typename TEnum>
	TEnum ParseEnumValue(const FString& Value, const TEnum DefaultValue)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		if (!Enum || IsUnsetValue(Value))
		{
			return DefaultValue;
		}

		const int64 Parsed = Enum->GetValueByNameString(TrimCell(Value));
		return Parsed == INDEX_NONE
			? DefaultValue
			: static_cast<TEnum>(Parsed);
	}

	// 쉼표로 구분된 Enum 이름을 배열로 변환합니다.
	template <typename TEnum>
	TArray<TEnum> ParseEnumArray(const FString& Value)
	{
		TArray<TEnum> Result;
		TArray<FString> Tokens;
		Value.ParseIntoArray(Tokens, TEXT(","), true);

		const UEnum* Enum = StaticEnum<TEnum>();
		if (!Enum)
		{
			return Result;
		}

		for (const FString& Token : Tokens)
		{
			const int64 Parsed = Enum->GetValueByNameString(TrimCell(Token));
			if (Parsed != INDEX_NONE)
			{
				Result.Add(static_cast<TEnum>(Parsed));
			}
		}
		return Result;
	}

	// 지정한 구분자로 문자열 배열을 만듭니다.
	inline TArray<FString> ParseStringArray(
		const FString& Value,
		const FString& Delimiter = TEXT(","))
	{
		TArray<FString> Result;
		if (IsUnsetValue(Value) || Delimiter.IsEmpty())
		{
			return Result;
		}

		TArray<FString> Tokens;
		Value.ParseIntoArray(Tokens, *Delimiter, true);
		for (const FString& Token : Tokens)
		{
			if (!IsUnsetValue(Token))
			{
				Result.Add(TrimCell(Token));
			}
		}
		return Result;
	}

	// 배열에 잘못된 값이 있으면 빈 배열을 반환합니다.
	inline TArray<int32> ParseIntArray(
		const FString& Value,
		const FString& Delimiter = TEXT(","))
	{
		TArray<int32> Result;
		for (const FString& Token : ParseStringArray(Value, Delimiter))
		{
			int32 Parsed = 0;
			if (!Detail::TryParseInt(Token, Parsed))
			{
				return {};
			}
			Result.Add(Parsed);
		}
		return Result;
	}

	// 배열에 잘못된 값이 있으면 빈 배열을 반환합니다.
	inline TArray<float> ParseFloatArray(
		const FString& Value,
		const FString& Delimiter = TEXT(","))
	{
		TArray<float> Result;
		return Detail::TryParseFloatArray(Value, Result, Delimiter)
			? Result
			: TArray<float>();
	}

	// UE 문자열 또는 R,G,B,A 형식을 색상으로 변환합니다.
	inline FLinearColor ParseLinearColorValue(
		const FString& Value,
		const FLinearColor& DefaultValue)
	{
		if (IsUnsetValue(Value))
		{
			return DefaultValue;
		}

		FLinearColor Parsed;
		if (Parsed.InitFromString(TrimCell(Value)))
		{
			return Parsed;
		}

		TArray<float> Components;
		if (!Detail::TryParseFloatArray(Value, Components)
			|| Components.Num() < 3
			|| Components.Num() > 4)
		{
			return DefaultValue;
		}

		return FLinearColor(
			Components[0],
			Components[1],
			Components[2],
			Components.IsValidIndex(3) ? Components[3] : 1.0f);
	}

	// UE 문자열 또는 X,Y 형식을 FVector2D로 변환합니다.
	inline FVector2D ParseVector2DValue(
		const FString& Value,
		const FVector2D& DefaultValue)
	{
		FVector2D Parsed;
		if (!IsUnsetValue(Value) && Parsed.InitFromString(TrimCell(Value)))
		{
			return Parsed;
		}

		TArray<float> Components;
		return Detail::TryParseFloatArray(Value, Components)
			&& Components.Num() == 2
			? FVector2D(Components[0], Components[1])
			: DefaultValue;
	}

	// UE 문자열 또는 X,Y,Z 형식을 FVector로 변환합니다.
	inline FVector ParseVectorValue(
		const FString& Value,
		const FVector& DefaultValue)
	{
		FVector Parsed;
		if (!IsUnsetValue(Value) && Parsed.InitFromString(TrimCell(Value)))
		{
			return Parsed;
		}

		TArray<float> Components;
		return Detail::TryParseFloatArray(Value, Components)
			&& Components.Num() == 3
			? FVector(Components[0], Components[1], Components[2])
			: DefaultValue;
	}

	// UE 문자열 또는 Pitch,Yaw,Roll 형식을 FRotator로 변환합니다.
	inline FRotator ParseRotatorValue(
		const FString& Value,
		const FRotator& DefaultValue)
	{
		FRotator Parsed;
		if (!IsUnsetValue(Value) && Parsed.InitFromString(TrimCell(Value)))
		{
			return Parsed;
		}

		TArray<float> Components;
		return Detail::TryParseFloatArray(Value, Components)
			&& Components.Num() == 3
			? FRotator(Components[0], Components[1], Components[2])
			: DefaultValue;
	}

	// UE Transform 문자열을 변환합니다.
	inline FTransform ParseTransformValue(
		const FString& Value,
		const FTransform& DefaultValue)
	{
		FTransform Parsed;
		return !IsUnsetValue(Value) && Parsed.InitFromString(TrimCell(Value))
			? Parsed
			: DefaultValue;
	}

	// 빈 값은 NAME_None으로 변환합니다.
	inline FName ParseNameValue(const FString& Value)
	{
		return IsUnsetValue(Value) ? NAME_None : FName(*TrimCell(Value));
	}

	// 쉼표로 구분된 이름을 FName 배열로 변환합니다.
	inline TArray<FName> ParseNameArray(const FString& Value)
	{
		TArray<FName> Result;
		for (const FString& Token : ParseStringArray(Value))
		{
			Result.Add(FName(*Token));
		}
		return Result;
	}

	// {0} 위치에 RowName을 넣어 에셋 이름을 만듭니다.
	inline FString MakeGeneratedAssetName(
		const FString& NameFormat,
		const FName RowName)
	{
		return FString::Format(*NameFormat, { RowName.ToString() });
	}

	// 폴더와 이름 형식을 기준으로 DataAsset을 찾거나 생성합니다.
	template <typename TDataAsset>
	TDataAsset* GetOrCreateDataAsset(
		const FString& FolderPath,
		const FString& NameFormat,
		const FName RowName)
	{
		if (IsUnsetValue(FolderPath)
			|| IsUnsetValue(NameFormat)
			|| RowName.IsNone())
		{
			return nullptr;
		}

		return UPathDataLoadHelper::GetOrCreateAsset<TDataAsset>(
			FolderPath,
			MakeGeneratedAssetName(NameFormat, RowName));
	}

	// 조건에 맞는 첫 번째 에셋의 Object 경로를 반환합니다.
	inline FString FindObjectReferencePath(
		const FString& FolderPath,
		const FString& ObjectNameFormat,
		const FName RowName)
	{
		if (IsUnsetValue(FolderPath)
			|| IsUnsetValue(ObjectNameFormat)
			|| RowName.IsNone())
		{
			return FString();
		}

		const TArray<FAssetData> Assets =
			UPathDataLoadHelper::GetAssetsByPathFilter(
				FolderPath,
				MakeGeneratedAssetName(ObjectNameFormat, RowName));
		return Assets.IsEmpty()
			? FString()
			: Assets[0].GetSoftObjectPath().ToString();
	}

	// 조건에 맞는 첫 번째 Blueprint의 생성 클래스 경로를 반환합니다.
	inline FString FindBlueprintClassReferencePath(
		const FString& FolderPath,
		const FString& ClassNameFormat,
		const FName RowName)
	{
		const FString ObjectPath = FindObjectReferencePath(
			FolderPath,
			ClassNameFormat,
			RowName);
		return ObjectPath.IsEmpty()
			? FString()
			: FString::Printf(TEXT("%s_C"), *ObjectPath);
	}

	// 조건에 맞는 Blueprint 생성 클래스를 Soft Pointer로 반환합니다.
	template <typename TClass>
	TSoftClassPtr<TClass> FindBlueprintClass(
		const FString& FolderPath,
		const FString& ClassNameFormat,
		const FName RowName)
	{
		const FString ClassPath = FindBlueprintClassReferencePath(
			FolderPath,
			ClassNameFormat,
			RowName);
		return ClassPath.IsEmpty()
			? TSoftClassPtr<TClass>()
			: TSoftClassPtr<TClass>(FSoftObjectPath(ClassPath));
	}

	// 조건에 맞는 에셋을 Soft Pointer로 반환합니다.
	template <typename TObjectType>
	TSoftObjectPtr<TObjectType> FindObject(
		const FString& FolderPath,
		const FString& ObjectNameFormat,
		const FName RowName)
	{
		const FString ObjectPath = FindObjectReferencePath(
			FolderPath,
			ObjectNameFormat,
			RowName);
		return ObjectPath.IsEmpty()
			? TSoftObjectPtr<TObjectType>()
			: TSoftObjectPtr<TObjectType>(FSoftObjectPath(ObjectPath));
	}
}
