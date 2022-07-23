// Copyright seiko_dev. All Rights Reserved.

#include "FakeBloomUI_Builder.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "FakeBloomUI.h"
#include "Engine/TextureRenderTarget2D.h"

UFakeBloomUI_Builder::UFakeBloomUI_Builder()
    : TargetWidget(nullptr)
    , BuildParameter(nullptr)
    , PaintParameter(nullptr)
{
}

void UFakeBloomUI_Builder::SetParameters(FFakeBloomUI_BuildParameter* InBuild, FFakeBloomUI_PaintParameter* InPaint)
{
    BuildParameter = InBuild;
    PaintParameter = InPaint;
}

bool UFakeBloomUI_Builder::DrawWidgetToTarget(UTextureRenderTarget2D* Target,
                                              class UWidget* WidgetToRender,
                                              const FFakeBloomUI_PreProcessArgs& PreProcessArgs,
                                              int32 Overhang,
                                              bool UseGamma,
                                              bool UpdateImmediate) const
{
    const FVector2D& LocalSize = PreProcessArgs.Geometry.GetLocalSize();

    if (!WidgetToRender)
    {
        UE_LOG(LogTemp, Warning, TEXT("DrawWidgetToTarget Fail : WidgetToRender is empty!"));
        return false;
    }
    if (LocalSize.X < 0 || LocalSize.Y < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("DrawWidgetToTarget Fail : LocalSize is 0 or less!"));
        return false;
    }
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("DrawWidgetToTarget Fail : Target is empty!"));
        return false;
    }

    FWidgetRenderer* WidgetRenderer = new FWidgetRenderer(UseGamma, false);
    check(WidgetRenderer);

    {
        const FVector2D DrawOffset(Overhang, Overhang);
        WidgetRenderer->SetIsPrepassNeeded(false); // Paintで先に描画済み(Layout計算済み)なのでPrepass不要

#if 1 /* Niagara UI Renderer対応実装 */
        // やってる事のメモ
        // https://github.com/seiko-dev/UI_Blooman/issues/29

        const TSharedRef<SWidget>& Content = WidgetToRender->TakeWidget();
        const float Scale = PreProcessArgs.Geometry.GetAccumulatedLayoutTransform().GetScale();
        const FVector2D RectLT(PreProcessArgs.CullingRect.Left, PreProcessArgs.CullingRect.Top);
        const FVector2D AbsolutePos = PreProcessArgs.Geometry.GetAccumulatedRenderTransform().GetTranslation();

        // ContentPos部分の距離は、Widget DesingerがZoomしている場合に拡縮がかかるので、打ち消しておく。
        FVector2D ContentPos = (AbsolutePos - RectLT) / Scale;

        WidgetRenderer->ViewOffset = DrawOffset - ContentPos;

        // OffsetしたCanvas
        TSharedRef<SConstraintCanvas> Canvas = SNew(SConstraintCanvas)
            + SConstraintCanvas::Slot()
            .Anchors(FAnchors(0, 0, 1, 1))
            .Offset(FMargin(ContentPos.X, ContentPos.Y, 0, 0))
            .Alignment(FVector2D(0, 0))
            [
                Content
            ];
#if 1
        WidgetRenderer->DrawWidget(Target, Canvas, ContentPos + LocalSize, 0.0f);

#else
        // https://github.com/seiko-dev/UI_Blooman/issues/32
        // ↑に取り組む時に使いそうなので残しておく
        UE_LOG(LogTemp, Log, TEXT("%s:     a %s"), UTF8_TO_TCHAR(__func__), *AbsolutePos.ToString());
        UE_LOG(LogTemp, Log, TEXT("%s:     c %s"), UTF8_TO_TCHAR(__func__), *ContentPos.ToString());
        UE_LOG(LogTemp, Log, TEXT("%s:     L %s"), UTF8_TO_TCHAR(__func__), *LocalSize.ToString());
        UE_LOG(LogTemp, Log, TEXT("%s:     w %s"), UTF8_TO_TCHAR(__func__), *WindowGeometry.GetLocalSize().ToString());
        UE_LOG(LogTemp, Log, TEXT("%s: "), UTF8_TO_TCHAR(__func__));

        // テクスチャ自体は等倍で描く必要があるので、Scaleは常に1.0。
        FGeometry WindowGeometry = FGeometry::MakeRoot(ContentPos + LocalSize, FSlateLayoutTransform(1.0));

        // Geometryと一致する大きさ
        FSlateRect WindowClipRect = WindowGeometry.GetLayoutBoundingRect();

        // 一時的に付け替えるので親を覚えておく
        TSharedPtr<SWidget> OldParent = Content->GetParentWidget();

        TSharedRef<SVirtualWindow> Window = SNew(SVirtualWindow).Size(WindowGeometry.GetLocalSize());
        Window->SetContent(Canvas);

        WidgetRenderer->DrawWindow(
            Target->GameThread_GetRenderTargetResource(),
            *MakeUnique<FHittestGrid>(),
            Window,
            WindowGeometry,
            WindowClipRect,
            0.0f);

        // 付け替えた親戻し
        if (OldParent.IsValid())
        {
            Content->AssignParentWidget(OldParent);
        }
#endif


#else /* シンプル版 */
        WidgetRenderer->ViewOffset = DrawOffset;
        WidgetRenderer->DrawWidget(Target, WidgetToRender->TakeWidget(), LocalSize, 0.0);

#endif
        FlushRenderingCommands();
        BeginCleanup(WidgetRenderer);


        if (UpdateImmediate) {
            Target->UpdateResourceImmediate(false);
        }
    }

    return true;
}

int32 UFakeBloomUI_Builder::GetRenderTargetMipMapNum(UTextureRenderTarget2D* Target)
{
    if (Target) {
        return Target->GetNumMips();
    }
    return -1;
}

const FFakeBloomUI_BuildParameter& UFakeBloomUI_Builder::GetBuildParameter() const
{
    if (BuildParameter) {
        return *BuildParameter;
    }

    ensure(0);
    static FFakeBloomUI_BuildParameter Dummy;
    return Dummy;
}

const FFakeBloomUI_PaintParameter& UFakeBloomUI_Builder::GetPaintParameter() const
{
    if (PaintParameter) {
        return *PaintParameter;
    }

    ensure(0);
    static FFakeBloomUI_PaintParameter Dummy;
    return Dummy;
}

bool UFakeBloomUI_Builder::IsDesignTime() const
{
    if (TargetWidget) {
        return TargetWidget->IsDesignTime();
    }
    return false;
}

class UWorld* UFakeBloomUI_Builder::GetWorld() const
{
    if (!HasAnyFlags(RF_ClassDefaultObject) // CDOでは無効
        && ensureMsgf(GetOuter(), TEXT("%s has a null OuterPrivate in GetWorld()"), *GetFullName()) // Outerあるよね？
        && !GetOuter()->HasAnyFlags(RF_BeginDestroyed)  // Outer死にかけてたら無効
        && !GetOuter()->IsUnreachable()) // Outerない事になってたら無効
    {
        return GetOuter()->GetWorld();
    }
    return nullptr;
}