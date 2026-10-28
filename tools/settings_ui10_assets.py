"""Source-sized Settings and single-player artwork for the shared NSP2/39 append.

Every control starts from its pinned source texture. The old112x39 tab and
55x55 menu icons are never used as resize inputs. Source TextureRect mode5
keeps the image aspect inside the projected authored control rectangle.
"""
from PIL import Image
import content_tiles as tiles
from patch_feedback_assets import write_image
from patch_ui7_assets import authored_tint
from world_scene_assets import load_scene_texture

VERSION=39
IMAGE_COUNT=22

def build_images(pack, cache, base_aux):
    scenes={name:pack.read(name).decode() for name in (
        'scenes/screens/settings_tab.tscn','scenes/ui/tickbox.tscn',
        'scenes/screens/custom_run/modifier_tickbox.tscn','scenes/ui/submenu_button.tscn',
        'scenes/screens/singleplayer_submenu.tscn','scenes/screens/paginator.tscn')}
    for name, token in (
        ('scenes/screens/settings_tab.tscn','custom_minimum_size = Vector2(256, 90)'),
        ('scenes/ui/tickbox.tscn','scale = Vector2(0.8, 0.8)'),
        ('scenes/screens/custom_run/modifier_tickbox.tscn','custom_minimum_size = Vector2(48, 48)'),
        ('scenes/ui/submenu_button.tscn','offset_left = -13.0'),
        ('scenes/ui/submenu_button.tscn','offset_right = 19.0'),
        ('scenes/screens/paginator.tscn','custom_minimum_size = Vector2(64, 64)')):
        tiles.require(token in scenes[name], 'Pinned setup geometry changed: '+name+' / '+token)
    rows=[]
    def add(key, source, size, source_control, *, hsv=None, modulate=None, fit=True, flip=False, rotate=False):
        original=load_scene_texture(pack,source).convert('RGBA')
        art=authored_tint(original,hsv,modulate) if hsv or modulate else original
        if flip:art=art.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        if rotate:art=art.transpose(Image.Transpose.ROTATE_90)
        if fit:
            factor=min(size[0]/art.width,size[1]/art.height)
            fitted=(max(1,round(art.width*factor)),max(1,round(art.height*factor)))
            image=Image.new('RGBA',size)
            image.alpha_composite(art.resize(fitted,Image.Resampling.LANCZOS),((size[0]-fitted[0])//2,(size[1]-fitted[1])//2))
        else:image=art.resize(size,Image.Resampling.LANCZOS)
        row=write_image(cache,'source-setup-'+key,image)
        row.update(key=key,auxiliary=base_aux+len(rows),source=source,source_size=list(original.size),
                   source_control_size=list(source_control),size=list(size),viewport_scale=[4,21],
                   texture_rect_mode=5 if fit else 0,hsv=hsv,modulate=modulate,flip_h=flip,rotate90=rotate)
        rows.append(row)
    add('settings-tab','images/packed/common_ui/settings_tab_selected.png',(49,17),(256,90))
    add('settings-tab-stroke','images/packed/common_ui/settings_tab_stroke.png',(49,17),(256,90),modulate=(.3648,.9104,.96,.752941))
    for pixels, source_size, prefix in ((10,51.2,'settings'),(7,38.4,'modifier')):
        for state in ('ticked','unticked'):
            add(prefix+'-'+state,'images/atlases/ui_atlas.sprites/checkbox_'+state+'.tres',(pixels,pixels),(source_size,source_size))
    arrow='images/packed/common_ui/settings_tiny_left_arrow.png'
    add('settings-arrow-left',arrow,(9,9),(48,48),hsv=(1,1,.9))
    add('settings-arrow-right',arrow,(9,9),(48,48),hsv=(1,1,.9),flip=True)
    add('settings-arrow-down',arrow,(5,5),(26,26),rotate=True)
    settings=pack.read('scenes/screens/settings_screen.tscn').decode()
    import re
    def material(identifier):
        block=settings.split('[sub_resource type="ShaderMaterial" id="'+identifier+'"]',1)[1].split('[',1)[0]
        return tuple(float(re.search(r'shader_parameter/'+name+r' = ([0-9.]+)',block).group(1)) for name in ('h','s','v'))
    for name, identifier in (('feedback','ShaderMaterial_prf3m'),('credits','ShaderMaterial_j50rd'),('reset','ShaderMaterial_e4bt1')):
        add('settings-'+name,'images/ui/reward_screen/reward_skip_button.png',(61,12),(320,64),hsv=material(identifier))
    add('custom-randomize','images/ui/reward_screen/reward_item_button.png',(38,10),(199,51),fit=False)
    panel='images/packed/common_ui/submenu_panel.png'
    for name,hue,saturation,value in (('standard',0,.8,.8),('daily',.4800000228,.8,.7),('custom',.930000044175,1.200000057,.65)):
        add(name+'-panel',panel,(69,134),(362,705),hsv=(hue,saturation,value),fit=False)
        # NSubmenuButton.OnFocus always sets V=1, not defaultV+.2.
        add(name+'-panel-focus',panel,(69,134),(362,705),hsv=(hue,saturation,1),fit=False)
        add(name+'-icon','images/ui/main_menu/submenu_'+name+'.png',(54,41),(284,216))
    tiles.require(len(rows)==IMAGE_COUNT,'Settings/setup source image count changed')
    return rows
