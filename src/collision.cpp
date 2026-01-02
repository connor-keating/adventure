
bool point_in_rect(fvec2 point, fvec2 box_pos, fvec2 box_shape)
{
  f32 x1 = box_pos.x - box_shape.x;
  f32 y1 = box_pos.y - box_shape.y;
  f32 x2 = box_pos.x + box_shape.x;
  f32 y2 = box_pos.y + box_shape.y;
  bool xcheck = (point.x >= x1) && (point.x <= x2);
  bool ycheck = (point.y >= y1) && (point.y <= y2);
  return (xcheck && ycheck);
}
