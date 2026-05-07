#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemDataAsset.h"
#include "ItemDataStructure.generated.h"

USTRUCT(BlueprintType)
struct FItemDataStructure : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UItemDataAsset* ItemDataAsset = nullptr;
};