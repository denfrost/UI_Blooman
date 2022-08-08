// Copyright seiko_dev. All Rights Reserved.

#include "FakeBloomUI_Painter.h"
#include "FakeBloomUI_Parameter.h"
#include "FakeBloomUI.h"

UFakeBloomUI_Painter::UFakeBloomUI_Painter()
{
}

void UFakeBloomUI_Painter::DrawImageToCenter(FPaintContext& Context,
                                             int32 OverhangX,
                                             int32 OverhangY,
                                             const FVector2D& InSizeScale,
                                             const FSlateBrush& Brush)
{
    FVector2D Size = (Context.AllottedGeometry.GetLocalSize() + 2*FVector2D(OverhangX, OverhangY)) * InSizeScale;
    FVector2D Position = (Context.AllottedGeometry.GetLocalSize() - Size) * 0.5f;

    FSlateDrawElement::MakeBox(
        Context.OutDrawElements,
        Context.MaxLayer,
        Context.AllottedGeometry.ToPaintGeometry(Position, Size),
        &Brush);

    Context.MaxLayer++;
}

void UFakeBloomUI_Painter::SetOpacity(float InOpacity)
{
    Opacity = InOpacity;
}

bool UFakeBloomUI_Painter::IsUsingValidTexture() const
{
    if (FakeBloomUI) {
        return FakeBloomUI->BaseParameter.IsUsingValidTexture();
    }
    return false;
}

const FFakeBloomUI_BaseParameter& UFakeBloomUI_Painter::GetBaseParameter() const
{
    if (FakeBloomUI) {
        return FakeBloomUI->BaseParameter;
    }

    ensure(0);
    static FFakeBloomUI_BaseParameter Dummy;
    return Dummy;
}

class UWorld* UFakeBloomUI_Painter::GetWorld() const
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
