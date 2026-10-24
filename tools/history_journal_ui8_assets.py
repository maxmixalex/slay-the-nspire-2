"""Original NMapPointHistoryHoverTip sprite-font icons; no private pixels."""
from PIL import Image
from content_tiles import require
from patch_feedback_assets import write_image
from world_scene_assets import load_scene_texture

BASE_AUX, COUNT, VERSION = 2045, 4, 30
IMAGE_SPECS = tuple((name, 'images/packed/sprite_fonts/'+name+'_icon.png', (5,5))
                   for name in ('gold','card','chest','potion'))

def build_images(pack,cache,base=BASE_AUX):
    require(base==BASE_AUX,'History journal image order changed')
    rows=[]
    for name,source,size in IMAGE_SPECS:
        original=load_scene_texture(pack,source).convert('RGBA')
        require(original.size==(24,24),'Pinned original history icon size changed')
        image=original.resize(size,Image.Resampling.LANCZOS)
        key='history-journal-'+name
        row=write_image(cache,key,image)
        row.update(key=key,source=source,source_size=list(original.size),auxiliary=base+len(rows))
        rows.append(row)
    return rows
