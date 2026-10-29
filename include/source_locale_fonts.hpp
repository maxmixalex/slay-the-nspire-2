#pragma once
namespace spire {
// MegaLabel.RefreshFont and MegaRichTextLabel.RefreshFont replace their normal
// face with FontType.Regular when FontManager.NeedsFontSubstitution is true.
// FontManager substitutes zhs, zht, jpn, kor, pol, rus and tha. This rule
// concerns the ordinary label face, even when the scene authors it as Bold.
// Rich [b]/[i] spans and explicit post-Ready overrides remain separate roles.
constexpr unsigned source_label_face(bool authored_fonts,unsigned authored){return authored_fonts?authored:0u;}
}
