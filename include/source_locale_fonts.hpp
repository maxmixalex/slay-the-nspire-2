#pragma once
namespace spire {
// MegaLabel.RefreshFont and MegaRichTextLabel.RefreshFont replace their normal
// face with FontType.Regular when FontManager.NeedsFontSubstitution is true.
// The pinned source substitutes zhs (Noto Sans Mono CJK), but not eng. Rich
// bold/italic spans and explicit business-code overrides are separate roles.
constexpr unsigned source_label_face(bool english,unsigned authored){return english?authored:0u;}
}
