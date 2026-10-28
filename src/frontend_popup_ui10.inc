// NGenericPopup -> NVerticalPopup -> NPopupYesNoButton, reflowed for the LCD.
static void native_frontend_popup(Canvas&c,const char*header,const char*body,bool erase_profile){
 using namespace popup_layout;auto&a=c.a;const bool original=a.content.has_speech_bubble_assets();
 c.ui(original?I10_PopupVertical:I6_PopupVertical,X,Y,W,H);
 const unsigned face=source_label_face(a.content.original_label_fonts(),5);Canvas title=c;title.font_face=uint8_t(face);
 const int title_size=12;
 profile_text_box(c,header,X+10,HeaderY,W-20,29,{title_size,face,rgb(239,200,81),64,1,rgb(84,63,0),1,255,1,1},true,true);
 Canvas description=c;description.bold_face=5;
 profile_text_box(description,body,BodyX,BodyY,BodyW,BodyH,{10,0,rgb(255,246,226),64,0,0,0,255,1,0},true,true,{},true);
 for(unsigned i=0;i<2;++i){const bool yes=i!=0;const int x=button_x(yes);
  const char*label=frontend_text(a,"main_menu_ui",erase_profile?(yes?"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.delete":"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.cancel"):(yes?"GENERIC_POPUP.confirm":"GENERIC_POPUP.cancel"),yes?"确认":"取消");
  const bool selected=!a.pointer.visible&&int(a.frontend_choice)==int(i);
  const auto visual=c.control_visual(label,x,ButtonY,ButtonW,ButtonH,ButtonFamily::OriginalPopup,selected,true);
  const ProfileTransform transform{pivot_x(yes),PivotY,visual.scale};
  const int left=transform.horizontal(image_x(yes)),top=transform.vertical(ImageY);
  const int width=mx(1,transform.horizontal(image_x(yes)+ImageW)-left),height=mx(1,transform.vertical(ImageY+ImageH)-top);
  const int outline=original?(yes?I10_PopupConfirmOutline:I10_PopupCancelOutline):(yes?I6_PopupConfirmOutline:I6_PopupCancelOutline);
  Canvas edge=c;edge.tint_red=visual.hover?240:0;edge.tint_green=visual.hover?180:0;edge.tint_blue=0;
  edge.picture(a.content.ui_image(outline),left,top,width,height,visual.hover?255:128,visual.hover?presentation::Add:presentation::Normal);
  const int base_s= yes?1200:750,base_v=yes?1100:1200;
  const unsigned saturation=unsigned((base_s+int(visual.saturation)-1000)*1000/base_s);
  const unsigned value=unsigned((base_v+int(visual.value)-1000)*1000/base_v);
  const int image=original?(yes?I10_PopupConfirm:I10_PopupCancel):(yes?I6_PopupConfirm:I6_PopupCancel);
  small_material_icon(c,a.content.ui_image(image),left,top,width,height,int(saturation),value);
  profile_text_box(c,label,label_x(yes),LabelY,LabelW,LabelH,{10,face,rgb(255,246,226),64,1,yes?rgb(31,71,0):rgb(89,18,0),1,255,1,1},true,true,transform);
  c.action(x,ButtonY,ButtonW,ButtonH,yes?UiAction::FrontendAccept:UiAction::FrontendCancel);
  if(!a.pointer.visible&&a.content.version()>=HistoryUi8ContentVersion){
   char key[20]{};input_key_label(a.input_bindings.keys[a.keyboard_only?1:0][unsigned(yes?InputCommand::Confirm:InputCommand::Cancel)],key,sizeof key);
   const int kx=x+ButtonW-22,ky=ButtonY+ButtonH-3,kw=mx(12,mn(32,c.width(key,8)+4));
   c.ui(I8_KeyboardKeycap,kx,ky,kw,12);profile_text_box(c,key,kx,ky,kw,12,{8,0,White,0},true,false);
  }
 }
}
