#include "GoogleSheetConfig.h"

#if WITH_EDITOR
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/DateTime.h"
#include "FileHelpers.h"

namespace
{
    bool IsUnsetConfigValue(const FString& Value)
    {
        const FString TrimmedValue = Value.TrimStartAndEnd();
        return TrimmedValue.IsEmpty()
            || TrimmedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase);
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
        TEXT("%s!%s:%s"),
        *PageName.TrimStartAndEnd(),
        *RangeFrom.TrimStartAndEnd(),
        *RangeTo.TrimStartAndEnd());
}

void UGoogleSheetConfig::Fetch()
{
    const FString ID = GetSpreadsheetID();
    const FString TrimmedPageName = PageName.TrimStartAndEnd();
    const FString TrimmedRangeFrom = RangeFrom.TrimStartAndEnd();
    const FString TrimmedRangeTo = RangeTo.TrimStartAndEnd();

    if (IsUnsetConfigValue(ID))
    {
        FetchStatus = EFetchStatus::Failed;
        LastMessage = TEXT("SheetURL is empty.");
        return;
    }

    if (IsUnsetConfigValue(TrimmedPageName))
    {
        FetchStatus = EFetchStatus::Failed;
        LastMessage = TEXT("PageName is empty.");
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

    const FString URL = FString::Printf(
        TEXT("https://docs.google.com/spreadsheets/d/%s/gviz/tq?sheet=%s&range=%s:%s"),
        *ID, *TrimmedPageName, *TrimmedRangeFrom, *TrimmedRangeTo);

    TSharedRef<IHttpRequest> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(URL);
    Req->SetVerb(TEXT("GET"));

    Req->OnProcessRequestComplete().BindWeakLambda(this,
        [this](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
        {
            const FDateTime Now = FDateTime::Now();
            LastFetchTime = Now.ToString(TEXT("%Y-%m-%d %H:%M:%S"));

            if (!bSuccess || !Response.IsValid())
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = TEXT("Network Error : No Response");
                return;
            }

            const int32 Code = Response->GetResponseCode();
            if (Code != 200)
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = FString::Printf(TEXT("HTTP Error: %d"), Code);
                return;
            }

            FString RawContent = Response->GetContentAsString();
            if (RawContent.TrimStartAndEnd().IsEmpty())
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = TEXT("Response is empty.");
                return;
            }

            if (RawContent.Contains(TEXT("/*O_o*/")))
            {
                TArray<FString> SplitByMagic;
                RawContent.ParseIntoArray(SplitByMagic, TEXT("/*O_o*/"), true);
                if (SplitByMagic.Num() > 0)
                {
                    RawContent = SplitByMagic.Last().TrimStartAndEnd();
                }
            }

            if (!IsValid(DataParser))
            {
                FetchStatus = EFetchStatus::Failed;
                LastMessage = TEXT("DataParser is empty.");
                return;
            }

            FString ParseMessage;
            if (!DataParser->Parse(RawContent, ParseMessage))
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
                TEXT("Success - %d bytes received"), Response->GetContent().Num());
        });

    Req->ProcessRequest();
}
#endif
