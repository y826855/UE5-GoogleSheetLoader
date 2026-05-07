#include "GoogleSheetConfig.h"

#if WITH_EDITOR
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/DateTime.h"

FString UGoogleSheetConfig::GetSpreadsheetID() const
{
    // URL 형태면 ID만 추출: /spreadsheets/d/{ID}/
    if (SheetURL.Contains(TEXT("docs.google.com")))
    {
        FString Left, Right;
        if (SheetURL.Split(TEXT("/d/"), &Left, &Right))
        {
            FString ID;
            Right.Split(TEXT("/"), &ID, &Left);
            return ID;
        }
    }
    // 아니면 그냥 ID로 취급
    return SheetURL;
}

FString UGoogleSheetConfig::GetRangeString() const
{
    // PageName!RangeFrom:RangeTo 형태
    return FString::Printf(TEXT("%s!%s:%s"),
        *PageName, *RangeFrom, *RangeTo);
}

void UGoogleSheetConfig::Fetch()
{
    const FString ID    = GetSpreadsheetID();
    const FString Range = GetRangeString();

    if (ID.IsEmpty() || PageName.IsEmpty())
    {
        FetchStatus  = EFetchStatus::Failed;
        LastMessage  = TEXT("SheetURL Or PageName is Empty.");
        return;
    }

    FetchStatus = EFetchStatus::Loading;
    LastMessage = TEXT("Request...");

    // 공개 시트 Json으로 받아옴
    const FString URL = FString::Printf(
        TEXT("https://docs.google.com/spreadsheets/d/%s/gviz/tq?sheet=%s&range=%s:%s"),
        *ID, *PageName, *RangeFrom, *RangeTo);
    
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
            // "/*O_o*/" 라는 글자가 있으면 그 앞부분은 다 버린다!
            if (RawContent.Contains(TEXT("/*O_o*/")))
            {
                TArray<FString> SplitByMagic;
                RawContent.ParseIntoArray(SplitByMagic, TEXT("/*O_o*/"), true);
                if (SplitByMagic.Num() > 0)
                    RawContent = SplitByMagic.Last().TrimStartAndEnd();
            }

            //파싱 클래스 실행
            FString temp;
            DataParser->Parse(RawContent, temp);

            FetchStatus = EFetchStatus::Success;
            LastMessage = FString::Printf(
                TEXT("Success — %d bytes Receive"), Response->GetContent().Num());
        });

    Req->ProcessRequest();
}
#endif