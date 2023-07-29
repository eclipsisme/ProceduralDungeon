/*
 * MIT License
 *
 * Copyright (c) 2023 Benoit Pelletier
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "SProceduralDungeonEdModeWidget.h"
#include "EditorModeManager.h"
#include "EditorStyleSet.h"
#include "ISinglePropertyView.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SErrorText.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "FileHelpers.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ProceduralDungeonEdLog.h"
#include "ProceduralDungeonEditor.h"
#include "ProceduralDungeonEdMode.h"
#include "ProceduralDungeonEdModeToolkit.h"
#include "Room.h" // TODO: remove the need to include Room.h when including RoomLevel.h
#include "RoomLevel.h"
#include "RoomData.h"

void SProceduralDungeonEdModeWidget::Construct(const FArguments& InArgs, TSharedRef<FProceduralDungeonEdModeToolkit> InParentToolkit)
{
    ParentToolkit = InParentToolkit;
    FProceduralDungeonEdMode* EdMode = InParentToolkit->GetEditorMode();
    FText LevelName = FText::FromString(GetNameSafe(EdMode->World.Get()));

    FSlateFontInfo TitleFont = FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle");
    TitleFont.Size = 24;

    FSlateFontInfo SubTitleFont = FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle");
    SubTitleFont.Size = 16;

    TSharedPtr<SScrollBox> DataScrollBox = nullptr;

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .VAlign(VAlign_Top)
        .Padding(5.f)
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(LevelName)
            .Justification(ETextJustify::Center)
            .AutoWrapText(true)
            .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
            .Font(TitleFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.0f)
        [
            SAssignNew(LevelPropertyContainer, SBorder)
            .Visibility(this, &SProceduralDungeonEdModeWidget::ShowDetails)
            .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
        ]
        + SVerticalBox::Slot()
        .Padding(5.0f)
        .AutoHeight()
        [
            SAssignNew(Error, SErrorText)
        ]
        + SVerticalBox::Slot()
        .Padding(5.0f)
        [
            SNew(SBorder)
            .BorderImage(FEditorStyle::GetBrush("DetailsView.CollapsedCategory"))
            .BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f))
            .Padding(5.0f)
            .Visibility(this, &SProceduralDungeonEdModeWidget::ShowDataDetails)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .AutoHeight()
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    .HAlign(EHorizontalAlignment::HAlign_Fill)
                    [
                        SNew(STextBlock)
                        .Text(this, &SProceduralDungeonEdModeWidget::GetDataAssetName)
                        .Justification(ETextJustify::Center)
                        .Font(SubTitleFont)
                    ]
                    + SOverlay::Slot()
                    .HAlign(EHorizontalAlignment::HAlign_Left)
                    .VAlign(EVerticalAlignment::VAlign_Center)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Edit")))
                        .OnClicked(this, &SProceduralDungeonEdModeWidget::EditData)
                        .IsEnabled(this, &SProceduralDungeonEdModeWidget::IsValidRoomData)
                        .HAlign(EHorizontalAlignment::HAlign_Center)
                    ]
                    + SOverlay::Slot()
                    .HAlign(EHorizontalAlignment::HAlign_Right)
                    .VAlign(EVerticalAlignment::VAlign_Center)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("Save")))
                        .OnClicked(this, &SProceduralDungeonEdModeWidget::SaveData)
                        .IsEnabled(this, &SProceduralDungeonEdModeWidget::IsDataDirty)
                        .ButtonColorAndOpacity(this, &SProceduralDungeonEdModeWidget::GetSaveButtonColor)
                        .HAlign(EHorizontalAlignment::HAlign_Center)
                    ]
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                [
                    SAssignNew(DataScrollBox, SScrollBox)
                    //.IsEnabled(this, &SProceduralDungeonEdModeWidget::MatchingDataLevel)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .Padding(5.0f)
        .AutoHeight()
        [
            SNew(SBorder)
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Visibility(this, &SProceduralDungeonEdModeWidget::ShowNote)
            .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
            .Padding(0.0f)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Reparent Level Blueprint")))
                .OnClicked(this, &SProceduralDungeonEdModeWidget::ReparentLevelActor)
                .ButtonColorAndOpacity(this, &SProceduralDungeonEdModeWidget::GetReparentButtonColor)
                .HAlign(EHorizontalAlignment::HAlign_Center)
            ]
        ]
    ];

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // RoomData details view
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = false;
    DetailsViewArgs.bLockable = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    DataContentWidget = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    DataContentWidget->OnFinishedChangingProperties().AddLambda([this](const FPropertyChangedEvent& Event) { UpdateErrorText(); });
    DataScrollBox->AddSlot()
    [
        DataContentWidget.ToSharedRef()
    ];

    UpdateFromLevel();
}

void SProceduralDungeonEdModeWidget::UpdateFromLevel()
{
    FProceduralDungeonEdMode* EdMode = ParentToolkit.Pin()->GetEditorMode();
    Level = EdMode->Level;
    DungeonEd_LogInfo("Slate Editor Level: %s", *GetNameSafe(Level.Get()));

    // Check Level script actor validity
    if (!IsValidRoomLevel())
    {
        UpdateErrorText();
        return;
    }

    // RoomLevel Data property
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    TSharedPtr<ISinglePropertyView> SinglePropView = PropertyEditorModule.CreateSingleProperty(Level.Get(), "Data", {});
    FSimpleDelegate PropertyChangedDelegate = FSimpleDelegate::CreateSP(this, &SProceduralDungeonEdModeWidget::OnDataAssetChanged);
    SinglePropView->SetOnPropertyValueChanged(PropertyChangedDelegate);
    LevelPropertyContainer->SetContent(SinglePropView.ToSharedRef());

    OnDataAssetChanged();
}

void SProceduralDungeonEdModeWidget::OnDataAssetChanged()
{
    if (CurrentRoomData.IsValid())
        CurrentRoomData->OnPropertiesChanged.Remove(DataDelegateHandle);

    CurrentRoomData = Level.IsValid() ? Level->Data : nullptr;
    if (IsValidRoomData())
    {
        DataContentWidget->SetObject(CurrentRoomData.Get());
        DataDelegateHandle = CurrentRoomData.Get()->OnPropertiesChanged.AddLambda([this](URoomData* Data) { UpdateErrorText(); });
    }

    auto EdMode = ParentToolkit.Pin()->GetEditorMode();
    check(EdMode);
    EdMode->SetDefaultTool();

    UpdateErrorText();
}

FReply SProceduralDungeonEdModeWidget::ReparentLevelActor()
{
    auto EdMode = ParentToolkit.Pin()->GetEditorMode();
    auto World = EdMode->World;
    ULevelScriptBlueprint* LevelBlueprint = World->PersistentLevel->GetLevelScriptBlueprint();
    if (!IsValid(LevelBlueprint))
    {
        DungeonEd_LogInfo("ERROR: Can't Reparent Level Blueprint for an unknown reason.");
        return FReply::Unhandled();
    }

    LevelBlueprint->ParentClass = ARoomLevel::StaticClass();
    FKismetEditorUtilities::CompileBlueprint(LevelBlueprint);

    DungeonEd_LogInfo("Level Blueprint '%s' successfully reparented!", *LevelBlueprint->GetName());

    EdMode->UpdateLevel();
    UpdateFromLevel();

    return FReply::Unhandled();
}

FReply SProceduralDungeonEdModeWidget::EditData()
{
    UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    if (!IsValid(AssetEditorSubsystem))
        return FReply::Unhandled();

    AssetEditorSubsystem->OpenEditorForAsset(Level.Get()->Data);
    return FReply::Handled();
}

FReply SProceduralDungeonEdModeWidget::SaveData()
{
    if(!IsValidRoomData())
        return FReply::Unhandled();

    auto Result = FEditorFileUtils::PromptForCheckoutAndSave({ Level->Data->GetPackage() }, /*bCheckDirty = */true, /*bPromptToSave = */false);
    if(Result == FEditorFileUtils::EPromptReturnCode::PR_Success)
        DungeonEd_LogInfo("Successfully Saved Data Asset: '%s'", *GetNameSafe(Level->Data));
    return FReply::Handled();
}

FSlateColor SProceduralDungeonEdModeWidget::GetSaveButtonColor() const
{
    const FLinearColor& Default = FLinearColor::White;
    const FLinearColor& Highlight = FLinearColor::Green;
    return IsDataDirty() ? GetHighlightButtonColor(Highlight, Default) : Default;
}

FSlateColor SProceduralDungeonEdModeWidget::GetReparentButtonColor() const
{
    return GetHighlightButtonColor(FLinearColor::Green);
}

void SProceduralDungeonEdModeWidget::UpdateErrorText()
{
    if (!IsValidRoomLevel())
        Error->SetError(TEXT("Persistent Level is not a Room Level."));
    else if (!IsValidRoomData())
        Error->SetError(TEXT("Room Level has no Room Data set."));
    else if (!MatchingDataLevel())
        Error->SetError(TEXT("Level's Data and Data's Level do not match."));
    else 
        Error->SetError(FText::GetEmpty());
}

bool SProceduralDungeonEdModeWidget::IsValidRoomLevel() const
{
    return Level.IsValid();
}

bool SProceduralDungeonEdModeWidget::IsValidRoomData() const
{
    if (!IsValidRoomLevel())
        return false;
    return IsValid(Level->Data);
}

bool SProceduralDungeonEdModeWidget::MatchingDataLevel() const
{
    if (!IsValidRoomData())
        return false;
    return Level->Data->Level.GetUniqueID() == Level->GetWorld()->GetPathName();
}

bool SProceduralDungeonEdModeWidget::IsDataDirty() const
{
    if (!IsValidRoomData())
        return false;

    return Level->Data->GetPackage()->IsDirty();
}

EVisibility SProceduralDungeonEdModeWidget::ShowDetails() const
{
    return IsValidRoomLevel() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SProceduralDungeonEdModeWidget::ShowDataDetails() const
{
    return IsValidRoomData() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SProceduralDungeonEdModeWidget::ShowNote() const
{
    return IsValidRoomLevel() ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SProceduralDungeonEdModeWidget::GetDataAssetName() const
{
    if (!IsValidRoomData())
        return FText::GetEmpty();

    FString Dirty = IsDataDirty() ? "*" : "";
    return FText::FromString(GetNameSafe(Level->Data) + Dirty);
}

FLinearColor SProceduralDungeonEdModeWidget::GetHighlightButtonColor(const FLinearColor& HighlightColor, const FLinearColor& NormalColor, float Speed)
{
    uint32 ticks = FDateTime::Now().GetTicks(); // needs this line to avoid compiler optimization that prevent getting Now() each frame.
    float seconds = static_cast<float>(ticks) / ETimespan::TicksPerSecond;
    float t = FMath::Cos(Speed * seconds);
    return FMath::Lerp(NormalColor, HighlightColor, t);
}