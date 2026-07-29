// Copyright ROK2. Name-based dynamic delegate binding helper (إصلاح AddDynamic).
//
// المشكلة التي يحلها هذا الملف:
// `AddDynamic` ماكرو لا دالة — يتوسّع إلى:
//     __Internal_AddDynamic( Obj, FuncName, STATIC_FUNCTION_FNAME( TEXT( #FuncName ) ) )
// ومعامله الثاني في `__Internal_AddDynamic` هو مؤشر دالة عضو
// (TMethodPtrResolver<UserClass>::FMethodPtr). فكتابة:
//     Btn->OnClicked.AddDynamic(this, Handler);   // Handler من نوع FName
// تفشل مرتين:
//   1) خطأ ترجمة صريح — لا تحويل من FName إلى مؤشر دالة عضو.
//   2) وحتى لو تُرجمت، `#FuncName` يحوّل *اسم المعامل* نصّياً، فالاسم المربوط
//      يصير الحرفي "Handler" لا قيمة المتغيّر — ربط خاطئ أصلاً.
//
// الحل: مسار الربط بالاسم العام في المحرك — FScriptDelegate::BindUFunction ثم
// Add. صالح لأن FOnButtonClickedEvent يرث TMulticastScriptDelegate<FWeakObjectPtr>
// (وفيه Add(const FScriptDelegate&) عامّة)، وFScriptDelegate هو
// TScriptDelegate<FWeakObjectPtr> (وفيه BindUFunction عامّة). لا تبعية موديول
// جديدة ولا تعديل على Rok2.Build.cs.
//
// لماذا دالة مساعدة واحدة بدل تكرار الثلاث أسطر في كل موضع: الربط بالاسم
// يُحلّ في وقت التشغيل، فلو الاسم غير موجود أو غير UFUNCTION يفشل الربط
// **بصمت** ويصير الزر ميتاً بلا أي خطأ ترجمة أو تشغيل. الدالة تتحقق بـ
// FindFunction وتصرخ في التطوير، فتحوّل عطلاً صامتاً إلى تحذير مرئي.
//
// الاستخدام:
//     auto MakeBtn = [&](const FString& Label, const FName Handler) {
//         UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
//         Rok2BindClickByName(Btn, this, Handler);   // بدل AddDynamic(this, Handler)
//         ...
//     };
//     MakeBtn(TEXT("ترقية"), FName(TEXT("OnUpgradeClicked")));
//
// ملاحظة: لا تستخدم هذه الدالة عندما يكون المُعالج معروفاً وقت الترجمة —
// في تلك الحالة `AddDynamic(this, &UMyClass::OnFoo)` أفضل لأنه يُتحقق منه
// وقت الترجمة. هذه الدالة للحالة التي يكون فيها اسم المُعالج بياناً (FName
// تُمرَّر إلى lambda مشتركة تبني عدة أزرار).

#pragma once

#include "CoreMinimal.h"
// Components/Button.h يُعرّف FOnButtonClickedEvent عبر
// DECLARE_DYNAMIC_MULTICAST_DELEGATE، فيجرّ معه آلية FScriptDelegate كاملة —
// لا حاجة لتضمين UObject/ScriptDelegates.h صراحةً.
#include "Components/Button.h"

/**
 * يربط UButton::OnClicked بدالة UFUNCTION على Target معروفة بالاسم.
 *
 * @param Button        الزر المراد ربطه (يُتجاهل الاستدعاء إن كان null).
 * @param Target        الكائن المالك للمُعالج — عادةً `this`.
 * @param FunctionName  اسم UFUNCTION() بتوقيع void بلا معاملات.
 * @return true إذا تم الربط فعلاً؛ false إذا كان أي معامل غير صالح أو الدالة
 *         غير موجودة (وفي هذه الحالة يُسجَّل ensure في بناء التطوير).
 */
inline bool Rok2BindClickByName(UButton* Button, UObject* Target, const FName FunctionName)
{
	if (!Button || !Target || FunctionName.IsNone())
	{
		return false;
	}

	// الربط بالاسم يُحلّ في وقت التشغيل: لو الاسم غلط أو الدالة ليست UFUNCTION
	// فلن يحدث شيء عند الضغط ولن يظهر أي خطأ. نمنع هذا العطل الصامت هنا.
	if (!ensureMsgf(Target->FindFunction(FunctionName) != nullptr,
		TEXT("Rok2BindClickByName: '%s' ليست UFUNCTION() على %s — الزر لن يستجيب للضغط."),
		*FunctionName.ToString(), *Target->GetClass()->GetName()))
	{
		return false;
	}

	FScriptDelegate Delegate;
	Delegate.BindUFunction(Target, FunctionName);
	Button->OnClicked.Add(Delegate);
	return true;
}
