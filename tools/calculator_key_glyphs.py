"""Calculator control keycaps, independent of the optional TI key font.

The labels match include/input_bindings.hpp. The outlined shapes and arrows are
our calculator interface artwork; they are not TI's official font or original
game art. Label text uses the owner's extracted Source Code Pro face.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

LABELS = {46:'Del',47:'Ctrl',59:'Flag',97:'Pad',98:'Menu',99:'Home',100:'Esc',
          101:'Tab',103:'Shift',104:'Var',107:'Cat',126:'Doc',183:'Enter'}
LABELS.update({code:chr(code) for code in range(48,58)})


def key_image(font_dir: Path, symbol: int, pixels: int):
    official=font_dir/'TINspireKeysCX.ttf'
    if official.is_file():
        font=ImageFont.truetype(str(official),pixels*3)
        char=chr(symbol);box=font.getbbox(char)
        image=Image.new('L',(max(1,box[2]-box[0]),max(1,box[3]-box[1])))
        ImageDraw.Draw(image).text((-box[0],-box[1]),char,font=font,fill=255)
    else:
        # Supersampling makes the public key outlines readable at native size.
        height=(pixels+1)*4
        if symbol in (161,162,163,164):
            image=Image.new('L',(height,height));draw=ImageDraw.Draw(image)
            center=height//2;low=height//4;high=height-low-1
            points=[(high,low),(low,center),(high,high)]
            if symbol==162:points=[(height-1-x,y) for x,y in points]
            elif symbol==163:points=[(y,x) for x,y in points]
            elif symbol==164:points=[(y,height-1-x) for x,y in points]
            draw.line(points,fill=255,width=max(2,height//9),joint='curve')
        else:
            label=LABELS[symbol]
            font=ImageFont.truetype(str(font_dir/'source_code_pro_semibold.ttf'),max(8,height*2//3))
            box=font.getbbox(label);width=max(height,box[2]-box[0]+height//3)
            image=Image.new('L',(width,height));draw=ImageDraw.Draw(image)
            draw.rounded_rectangle((1,1,width-2,height-2),radius=max(2,height//7),
                                   outline=255,width=max(1,height//14))
            draw.text(((width-(box[2]-box[0]))//2-box[0],
                       (height-(box[3]-box[1]))//2-box[1]),label,font=font,fill=255)
    height=pixels+1;width=max(1,min(38,round(image.width*height/image.height)))
    return image.resize((width,height),Image.Resampling.LANCZOS)
