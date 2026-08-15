const fs = require('fs');
const path = require('path');

// 1. Fix Rok2CityWidget.cpp
{
  const file = path.resolve('Source/Rok2/Private/Rok2CityWidget.cpp');
  let content = fs.readFileSync(file, 'utf8');

  // Collapse legacy TopBarBorder in CityWidget since HudWidget renders the unified top bar
  content = content.replace(
    /TopBarBorder->SetBrushColor\(FLinearColor\(0\.02f, 0\.05f, 0\.12f, 0\.92f\)\);/,
    'TopBarBorder->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.12f, 0.92f));\n\t\tTopBarBorder->SetVisibility(ESlateVisibility::Collapsed);'
  );

  // Add ApplyFont to MakePanelTitle
  content = content.replace(
    /T->SetText\(FText::FromString\(Label\)\);\s*T->SetColorAndOpacity\(FSlateColor\(Color\)\);/,
    'T->SetText(FText::FromString(Label));\n\t\t\tT->SetColorAndOpacity(FSlateColor(Color));\n\t\t\tURok2Typography::ApplyFont(T, ERok2TextRole::TitleCompact);'
  );

  // Add ApplyFont to MakeIconBtn
  content = content.replace(
    /Txt->SetText\(FText::FromString\(Label\)\);\s*Txt->SetColorAndOpacity\(FSlateColor\(FLinearColor::White\)\);/,
    'Txt->SetText(FText::FromString(Label));\n\t\t\tTxt->SetColorAndOpacity(FSlateColor(FLinearColor::White));\n\t\t\tURok2Typography::ApplyFont(Txt, ERok2TextRole::Button);'
  );

  // Add ApplyFont to QueueTitle
  content = content.replace(
    /QueueTitle->SetColorAndOpacity\(FSlateColor\(FLinearColor\(0\.2f, 0\.8f, 1\.0f\)\)\);/,
    'QueueTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.8f, 1.0f)));\n\t\t\tURok2Typography::ApplyFont(QueueTitle, ERok2TextRole::TitleCompact);'
  );

  // Add ApplyFont to TrnBtnText
  content = content.replace(
    /TrnBtnText->SetText\(FText::FromString\(TEXT\("تدريب"\)\)\);/,
    'TrnBtnText->SetText(FText::FromString(TEXT("تدريب")));\n\t\tURok2Typography::ApplyFont(TrnBtnText, ERok2TextRole::Button);'
  );

  // Add ApplyFont to AllBtnText
  content = content.replace(
    /AllBtnText->SetText\(FText::FromString\(TEXT\("إنشاء"\)\)\);/,
    'AllBtnText->SetText(FText::FromString(TEXT("إنشاء")));\n\t\tURok2Typography::ApplyFont(AllBtnText, ERok2TextRole::Button);'
  );

  fs.writeFileSync(file, '\uFEFF' + content.replace(/^\uFEFF/, ''), 'utf8');
  console.log('Fixed Rok2CityWidget.cpp');
}

// 2. Fix Rok2HudWidget.cpp
{
  const file = path.resolve('Source/Rok2/Private/Rok2HudWidget.cpp');
  let content = fs.readFileSync(file, 'utf8');

  // Fix ConnStateText ApplyFont
  content = content.replace(
    /ConnStateText->SetColorAndOpacity\(FSlateColor\(Rok2HudStyle::Muted\)\);/,
    'ConnStateText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));\n\tURok2Typography::ApplyFont(ConnStateText, ERok2TextRole::Caption);'
  );

  // Fix SeasonText ApplyFont
  content = content.replace(
    /SeasonText->SetColorAndOpacity\(FSlateColor\(Rok2HudStyle::Gold\)\);/,
    'SeasonText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));\n\tURok2Typography::ApplyFont(SeasonText, ERok2TextRole::Caption);'
  );

  // Fix ZoneTimerText ApplyFont
  content = content.replace(
    /ZoneTimerText->SetColorAndOpacity\(FSlateColor\(Rok2HudStyle::Muted\)\);/,
    'ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));\n\tURok2Typography::ApplyFont(ZoneTimerText, ERok2TextRole::Caption);'
  );

  // Fix Lbl in BuildTopBar (Resource labels)
  content = content.replace(
    /Out->SetColorAndOpacity\(FSlateColor\(Rok2HudStyle::Ivory\)\);/,
    'Out->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory));\n\t\tURok2Typography::ApplyFont(Out, ERok2TextRole::Numeric);'
  );

  // Fix notif center header title
  content = content.replace(
    /Title->SetText\(FText::FromString\(TEXT\("مركز الإشعارات"\)\)\);\s*Title->SetColorAndOpacity\(FSlateColor\(Rok2HudStyle::Gold\)\);/,
    'Title->SetText(FText::FromString(TEXT("مركز الإشعارات")));\n\tTitle->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));\n\tURok2Typography::ApplyFont(Title, ERok2TextRole::TitleCompact);'
  );

  // Fix QueuesBox empty text ApplyFont
  content = content.replace(
    /Empty->SetText\(FText::FromString\(TEXT\("لا طوابير نشطة"\)\)\);\s*Empty->SetColorAndOpacity\(FSlateColor\(Rok2HudStyle::Muted\)\);/,
    'Empty->SetText(FText::FromString(TEXT("لا طوابير نشطة")));\n\t\tEmpty->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));\n\t\tURok2Typography::ApplyFont(Empty, ERok2TextRole::Caption);'
  );

  fs.writeFileSync(file, '\uFEFF' + content.replace(/^\uFEFF/, ''), 'utf8');
  console.log('Fixed Rok2HudWidget.cpp');
}

// 3. Ensure all Source/Rok2 files have UTF-8 BOM
const sourceDir = path.resolve('Source/Rok2');
function addBomRecursively(dir) {
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      addBomRecursively(fullPath);
    } else if (entry.name.endsWith('.cpp') || entry.name.endsWith('.h')) {
      const raw = fs.readFileSync(fullPath);
      // Check if has UTF-8 BOM
      if (raw[0] !== 0xEF || raw[1] !== 0xBB || raw[2] !== 0xBF) {
        const text = raw.toString('utf8');
        fs.writeFileSync(fullPath, '\uFEFF' + text, 'utf8');
        console.log('Added UTF-8 BOM to:', entry.name);
      }
    }
  }
}
addBomRecursively(sourceDir);
console.log('All source files checked for UTF-8 BOM');
