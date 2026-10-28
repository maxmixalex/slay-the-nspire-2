// Native preview holders retain the source 300x422 card size and centered
// positions; selection, cancellation and costs remain canonical commands.
static void deck_confirm_screen(Canvas&c){
    auto&a=c.a;if(!a.deck_candidate_valid()){a.deck_candidate=-1;return;}
    using namespace deck_selection_layout;
    const Card before=a.game.deck[a.deck_candidate];a.hit_count=0;
    Canvas backstop=c;backstop.opacity=uint8_t(div255(unsigned(backstop.opacity)*PreviewBackstop));backstop.rect(0,0,320,240,0);
    if(a.game.phase==Phase::Upgrade){
        card_selection_upgrade_preview(c,before,-1,a.deck_candidate);
    }else draw_card_face(c,before,SingleX,PreviewY,PreviewW,PreviewH,-2,-2,-1,a.deck_candidate);
    native_original_back(c,a.deck_selection.preview_opened);
    native_original_confirm(c,UiAction::ConfirmDeckChoice,a.deck_selection.preview_opened);
    deck_selection_prompt(c);deck_selection_upgrades(c);
}
