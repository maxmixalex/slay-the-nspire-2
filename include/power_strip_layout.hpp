#pragma once
namespace spire {
// NPowerContainer.UpdatePositions uses power.tscn's fixed48px cell width,
// including for the vertical step. Amount text never resizes the cell.
struct PowerStripLayout {
 int left=0,top=0,cell=1,columns=1,rows=0,width=0;
 static constexpr PowerStripLayout make(int x,int y,int container_width,int count,int cell_size)noexcept{
  PowerStripLayout out;out.left=x;out.top=y;out.cell=cell_size>0?cell_size:1;
  out.columns=(container_width+out.cell-1)/out.cell;if(out.columns<1)out.columns=1;
  const int two_rows=(count+1)/2;if(out.columns<two_rows)out.columns=two_rows;
  if(count>0){out.rows=(count+out.columns-1)/out.columns;out.width=out.cell*(count<out.columns?count:out.columns);}
  if(out.width>container_width)out.left-=(out.width-container_width)/2;
  return out;
 }
 constexpr int x(int index)const noexcept{return left+index%columns*cell;}
 constexpr int y(int index)const noexcept{return top+index/columns*cell;}
};
}
