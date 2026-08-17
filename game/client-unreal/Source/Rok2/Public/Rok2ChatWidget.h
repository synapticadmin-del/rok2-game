// Copyright ROK2. P6-T6: دردشة حية — قناتا المملكة والتحالف.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2Types.h"
#include "Rok2ChatWidget.generated.h"

class URok2Api;
class UScrollBox;
class UEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;

UCLASS()
class URok2ChatWidget : public UUserWidget, public IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/**
	 * P18-T5: إغلاق الدردشة.
	 *
	 * كان في ترويستها زر «_» للتصغير وحده: يطوي الرسائل وشريط الإدخال
	 * والتبويبات فيبقى شريط الترويسة على الشاشة إلى الأبد — ولا مسار إزالة في
	 * المشروع كله.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void CloseSelf();

	virtual void DismissLayer() override { CloseSelf(); }

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	URok2Api* Api;

	/** يُستدعى عند وصول رسالة جديدة — يضيفها للقائمة */
	UFUNCTION()
	void OnChatReceived(const FRok2ChatMessage& Msg);

	/** يحدّث عدّاد غير المقروء */
	UFUNCTION()
	void UpdateUnreadBadge();

protected:
	// ---- واجهة ----
	UPROPERTY(Transient) UScrollBox* MessageScroll;
	UPROPERTY(Transient) UVerticalBox* MessageVBox;
	UPROPERTY(Transient) UEditableTextBox* InputField;
	UPROPERTY(Transient) UButton* SendButton;
	UPROPERTY(Transient) UButton* KingdomTab;
	UPROPERTY(Transient) UButton* AllianceTab;
	UPROPERTY(Transient) UButton* MinimizeButton;
	UPROPERTY(Transient) UTextBlock* UnreadBadge;
	UPROPERTY(Transient) UHorizontalBox* InputBar;
	UPROPERTY(Transient) UVerticalBox* ContentVBox;

	// ---- حالة ----
	FString ActiveChannel = "kingdom";
	bool bMinimized = false;

	// ---- بناء الـ UMG ----
	void BuildWidgetTree();
	void AddMessageBubble(const FRok2ChatMessage& Msg);

	// ---- أحداث ----
	UFUNCTION() void OnSendClicked();
	UFUNCTION() void OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UFUNCTION() void OnKingdomTabClicked();
	UFUNCTION() void OnAllianceTabClicked();
	UFUNCTION() void OnMinimizeClicked();

	/** زر الإغلاق في الترويسة (P18-T5) — يمرّ بـ`CloseSelf`. */
	UFUNCTION() void OnCloseClicked();


	// ---- ألوان الحضارات ----
	static FLinearColor GetCivColor(const FString& Civ);

	void SwitchChannel(const FString& NewChannel);
};
