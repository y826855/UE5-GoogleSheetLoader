#include "GoogleSheetConfig.h"

#if WITH_EDITOR
#include "Parser/GoogleSheetTableData.h"
#include "Parser/GoogleSheetTsvReader.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "FileHelpers.h"

namespace
{
    bool IsUnsetConfigValue(const FString& Value)
    {
        const FString TrimmedValue = Value.TrimStartAndEnd();
        return TrimmedValue.IsEmpty()
            || TrimmedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase);
    }

    bool ReadResponseContent(
        const FHttpResponsePtr& Response,
        const bool bRequestSucceeded,
        FString& OutContent,
        FString& OutError)
    {
        OutContent.Reset();
        OutError.Reset();

        if (!bRequestSucceeded || !Response.IsValid())
        {
            OutError = TEXT("Network Error: No Response");
            return false;
        }

        const int32 ResponseCode = Response->GetResponseCode();
        if (ResponseCode != 200)
        {
            OutError = FString::Printf(
                TEXT("HTTP Error: %d"),
                ResponseCode);
            return false;
        }

        OutContent = Response->GetContentAsString();
        if (OutContent.TrimStartAndEnd().IsEmpty())
        {
            OutError = TEXT("Response is empty.");
            return false;
        }

        const FString TrimmedContent = OutContent.TrimStart();
        if (TrimmedContent.StartsWith(
                TEXT("<!DOCTYPE"),
                ESearchCase::IgnoreCase)
            || TrimmedContent.StartsWith(
                TEXT("<html"),
                ESearchCase::IgnoreCase))
        {
            OutError =
                TEXT("TSV 대신 HTML을 받았습니다. 시트 공개 설정과 gid를 확인하세요.");
            return false;
        }

        return true;
    }

    bool ConvertTsvToJson(
        const FString& Tsv,
        const FString& RangeFrom,
        const FString& RangeTo,
        FGoogleSheetTableData& OutTableData,
        FString& OutJson,
        FString& OutError)
    {
        if (!FGoogleSheetTsvReader::Parse(
            Tsv,
            RangeFrom,
            RangeTo,
            OutTableData,
            OutError))
        {
            return false;
        }

        return OutTableData.ToJson(OutJson, OutError);
    }

    bool SaveNormalizedJsonFile(
        const FString& ConfigPath,
        const FString& Json,
        FString& OutPath,
        FString& OutError)
    {
        const FString SaveDirectory = FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("GoogleSheetLoader"));
        IFileManager::Get().MakeDirectory(*SaveDirectory, true);

        const FString JsonFileName = FPaths::MakeValidFileName(
            ConfigPath,
            TEXT('_')) + TEXT(".json");
        OutPath = FPaths::Combine(SaveDirectory, JsonFileName);
        if (FFileHelper::SaveStringToFile(
            Json,
            *OutPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            return true;
        }

        OutError = FString::Printf(
            TEXT("JSON 저장에 실패했습니다: %s"),
            *OutPath);
        OutPath.Reset();
        return false;
    }
}

FString UGoogleSheetConfig::GetSpreadsheetID() const
{
    const FString TrimmedSheetURL = SheetURL.TrimStartAndEnd();
    if (IsUnsetConfigValue(TrimmedSheetURL))
    {
        return FString();
    }

    if (TrimmedSheetURL.Contains(TEXT("docs.google.com")))
    {
        FString Left, Right;
        if (TrimmedSheetURL.Split(TEXT("/d/"), &Left, &Right))
        {
            FString ID;
            Right.Split(TEXT("/"), &ID, &Left);
            return ID.TrimStartAndEnd();
        }
    }

    return TrimmedSheetURL;
}

FString UGoogleSheetConfig::GetRangeString() const
{
    return FString::Printf(
        TEXT("%s:%s"),
        *RangeFrom.TrimStartAndEnd(),
        *RangeTo.TrimStartAndEnd());
}

FString UGoogleSheetConfig::GetSheetGid() const
{
    const FString GidMarker = TEXT("gid=");
    const int32 GidStart = SheetURL.Find(
        GidMarker,
        ESearchCase::IgnoreCase,
        ESearchDir::FromStart);
    if (GidStart != INDEX_NONE)
    {
        const int32 ValueStart = GidStart + GidMarker.Len();
        int32 ValueEnd = ValueStart;
        while (ValueEnd < SheetURL.Len() && FChar::IsDigit(SheetURL[ValueEnd]))
        {
            ++ValueEnd;
        }

        if (ValueEnd > ValueStart)
        {
            return SheetURL.Mid(ValueStart, ValueEnd - ValueStart);
        }
    }

    // gid가 생략된 링크는 Google Sheets의 첫 번째 탭을 가리킵니다.
    return TEXT("0");
}

void UGoogleSheetConfig::Fetch()
{
    if (FetchStatus == EFetchStatus::Loading)
    {
        return;
    }

    const FString ID = GetSpreadsheetID();
    const FString Gid = GetSheetGid();
    const FString TrimmedRangeFrom = RangeFrom.TrimStartAndEnd();
    const FString TrimmedRangeTo = RangeTo.TrimStartAndEnd();

    if (IsUnsetConfigValue(ID))
    {
        FetchStatus = EFetchStatus::Failed;
        LastMessage = TEXT("SheetURL is empty.");
        return;
    }

    if (IsUnsetConfigValue(TrimmedRangeFrom) || IsUnsetConfigValue(TrimmedRangeTo))
    {
        FetchStatus = EFetchStatus::Failed;
        LastMessage = TEXT("RangeFrom or RangeTo is empty.");
        return;
    }

    if (!IsValid(DataParser))
    {
        FetchStatus = EFetchStatus::Failed;
        LastMessage = TEXT("DataParser is empty.");
        return;
    }

    FetchStatus = EFetchStatus::Loading;
    LastMessage = TEXT("Request...");
    LastNormalizedJsonPath.Reset();

    const FString URL = FString::Printf(
        TEXT("https://docs.google.com/spreadsheets/d/%s/export?format=tsv&gid=%s"),
        *ID,
        *Gid);

    TSharedRef<IHttpRequest> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(URL);
    Req->SetVerb(TEXT("GET"));

    Req->OnProcessRequestComplete().BindWeakLambda(this,
        [this, TrimmedRangeFrom, TrimmedRangeTo](
            FHttpRequestPtr,
            FHttpResponsePtr Response,
            bool bSuccess)
        {
            const FDateTime Now = FDateTime::Now();
            LastFetchTime = Now.ToString(TEXT("%Y-%m-%d %H:%M:%S"));

            FString RawContent;
            FString ErrorMessage;
            if (!ReadResponseContent(
                Response,
                bSuccess,
                RawContent,
                ErrorMessage))
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = ErrorMessage;
                return;
            }

            if (!IsValid(DataParser))
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = TEXT("DataParser is empty.");
                return;
            }

            FGoogleSheetTableData TableData;
            FString NormalizedJson;
            if (!ConvertTsvToJson(
                RawContent,
                TrimmedRangeFrom,
                TrimmedRangeTo,
                TableData,
                NormalizedJson,
                ErrorMessage))
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = ErrorMessage;
                return;
            }

            if (bSaveNormalizedJson
                && !SaveNormalizedJsonFile(
                    GetPathName(),
                    NormalizedJson,
                    LastNormalizedJsonPath,
                    ErrorMessage))
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = ErrorMessage;
                return;
            }

            FString ParseMessage;
            if (!DataParser->Parse(NormalizedJson, ParseMessage))
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = ParseMessage.IsEmpty()
                    ? TEXT("Parse failed.")
                    : ParseMessage;
                return;
            }

            if (bAutoSaveOnComplete)
            {
                FEditorFileUtils::SaveDirtyPackages(true, true, true);
            }

            FetchStatus = EFetchStatus::Success;
            LastMessage = FString::Printf(
                TEXT("Success - %d rows, %d columns"),
                TableData.Rows.Num(),
                TableData.Headers.Num());
        });

    if (!Req->ProcessRequest())
    {
        FetchStatus = EFetchStatus::Failed;
        LastMessage = TEXT("HTTP 요청을 시작하지 못했습니다.");
    }
}
#endif
