#include "SpriteListCtrl.h"
#include <Core/Core.h>

SpriteListCtrl::SpriteListCtrl() 
    : project(nullptr)
    , frame(nullptr)
    , selected_index(-1)
    , is_dragging(false)
    , sort_type(SORT_BY_NAME)  // Default sort by name
{
    AddFrame(WhiteFrame());
}

SpriteListCtrl::~SpriteListCtrl() {
}

void SpriteListCtrl::SetProject(const AnimationProject* project) {
    this->project = project;
    RefreshList();
}

void SpriteListCtrl::SetFrame(const AnimationFrame* frame) {
    this->frame = frame;
}

void SpriteListCtrl::RefreshList() {
    ApplyFilters();
    Refresh();
}

void SpriteListCtrl::SetFilterText(const String& text) {
    filter_text = text;
    ApplyFilters();
    Refresh();
}

void SpriteListCtrl::SetCategoryFilter(const String& category) {
    if (category == "All Categories") {
        category_filter.Clear();
    } else {
        category_filter = category;
    }
    ApplyFilters();
    Refresh();
}

bool SpriteListCtrl::LoadAndCacheTexture(const String& texture_path, Image& img) {
    if (texture_path.IsEmpty()) {
        return false;
    }
    
    // Check if the image is already cached
    int cache_idx = texture_cache.Find(texture_path);
    if (cache_idx >= 0) {
        img = texture_cache.Get(cache_idx);
        return true;
    }
    
    // Try to load the texture from the path
    // In a real implementation, we would load the actual image file
    // For now, we'll create a placeholder image with the first letter of the filename
    FileIn fi(AsString(texture_path));
    if (fi && fi.IsOpen()) {
        img = StreamRaster::LoadImage(fi);
        if (img) {
            // Add to cache, evicting oldest if at limit
            if (texture_cache.GetCount() >= cache_size_limit) {
                texture_cache.Remove(0); // Remove oldest (first) entry
            }
            texture_cache.Add(texture_path, img);
            return true;
        }
    }
    
    // If we couldn't load, create a placeholder
    img = Image::Arrow(); // Use a basic placeholder
    if (texture_cache.GetCount() >= cache_size_limit) {
        texture_cache.Remove(0); // Remove oldest (first) entry
    }
    texture_cache.Add(texture_path, img);
    return false;
}

void SpriteListCtrl::ClearTextureCache() {
    texture_cache.Clear();
}

void SpriteListCtrl::RefreshList() {
    ClearTextureCache(); // Clear cache when refreshing the list
    ApplyFilters();
    Refresh();
}

void SpriteListCtrl::ApplyFilters() {
    display_indices.Clear();
    
    if (!project) return;
    
    for (int i = 0; i < project->sprites.GetCount(); i++) {
        const Sprite& sprite = project->sprites[i];
        
        // Apply text filter
        if (!filter_text.IsEmpty()) {
            String search_text = ToLower(filter_text);
            String sprite_id_lower = ToLower(sprite.id);
            String sprite_name_lower = ToLower(sprite.name);
            
            if (sprite_id_lower.Find(search_text) < 0 && 
                sprite_name_lower.Find(search_text) < 0) {
                continue;
            }
        }
        
        // Apply category filter
        if (!category_filter.IsEmpty() && sprite.category != category_filter) {
            continue;
        }
        
        display_indices.Add(i);
    }
    
    // Sort the filtered results
    SortSprites();
}

int SpriteListCtrl::HitTest(Point pos) const {
    if (!project) return -1;
    
    int y = 0;
    int item_height = 40; // Fixed height for each item
    
    for (int i = 0; i < display_indices.GetCount(); i++) {
        Rect item_rect = RectC(0, y, GetSize().cx, item_height);
        if (item_rect.Contains(pos)) {
            return i; // Return the display index, not the project index
        }
        y += item_height;
    }
    
    return -1;
}

void SpriteListCtrl::DrawItem(Draw& w, int display_index, const Rect& rc) const {
    if (!project || display_index < 0 || display_index >= display_indices.GetCount()) {
        return;
    }
    
    int project_index = display_indices[display_index];
    const Sprite& sprite = project->sprites[project_index];
    
    // Draw background
    Color bg_color = (display_index == selected_index) ? LtBlue() : (display_index % 2 == 0 ? White() : SdkLightGray());
    w.DrawRect(rc, bg_color);
    
    // Check if this sprite is used in the current frame
    bool in_current_frame = false;
    if (frame) {
        for (int i = 0; i < frame->sprites.GetCount(); i++) {
            if (frame->sprites[i].sprite_id == sprite.id) {
                in_current_frame = true;
                break;
            }
        }
    }
    
    // Check if this sprite is used in any frame in the project
    bool sprite_is_used = false;
    for (int i = 0; i < project->frames.GetCount(); i++) {
        const Frame& frame_ref = project->frames[i];
        for (int j = 0; j < frame_ref.sprites.GetCount(); j++) {
            if (frame_ref.sprites[j].sprite_id == sprite.id) {
                sprite_is_used = true;
                break;
            }
        }
        if (sprite_is_used) break; // Found at least one usage
    }
    
    // Draw indicator if in current frame
    if (in_current_frame) {
        w.DrawRect(rc.left + 2, rc.top + 2, 4, 4, Green());
    }
    
    // Draw indicator if sprite is used in any frame (for dangling reference awareness)
    if (sprite_is_used) {
        w.DrawRect(rc.right - 6, rc.top + 2, 4, 4, Blue());
    }
    
    // Draw thumbnail if available
    int thumb_size = 32;
    Rect thumb_rect = RectC(rc.left + 4, rc.top + 4, thumb_size, thumb_size);
    
    Image img;
    if (!sprite.texture_path.IsEmpty() && LoadAndCacheTexture(sprite.texture_path, img)) {
        // Draw the loaded image, scaling it to fit the thumbnail area
        DrawImage(w, thumb_rect.left, thumb_rect.top, thumb_size, thumb_size, img);
    } else {
        // Draw a placeholder if no image is available
        w.DrawRect(thumb_rect, LtGray());
        // Draw the first letter of the sprite ID as placeholder
        if (!sprite.id.IsEmpty()) {
            w.DrawText(thumb_rect.left + 12, thumb_rect.top + 10, String() << sprite.id[0], Arial(14), DarkGray());
        }
    }
    
    // Draw sprite name (as main identifier)
    String displayName = !sprite.name.IsEmpty() ? sprite.name : sprite.id; // Use name if available, otherwise id
    w.DrawText(rc.left + 40, rc.top + 3, displayName, Arial(12), Black());
    
    // Draw sprite ID (as secondary info)
    w.DrawText(rc.left + 40, rc.top + 18, "ID: " + sprite.id, Arial(10), Gray());
    
    // Draw category and tags
    String category_tags = "Cat: " + sprite.category;
    if (sprite.tags.GetCount() > 0) {
        category_tags += " | Tags: ";
        for (int i = 0; i < min(3, sprite.tags.GetCount()); i++) { // Show first 3 tags
            category_tags += sprite.tags[i];
            if (i < min(3, sprite.tags.GetCount()) - 1) category_tags += ",";
        }
        if (sprite.tags.GetCount() > 3) {
            category_tags += "...";
        }
    }
    w.DrawText(rc.left + 40, rc.top + 30, category_tags, Arial(8), DarkGray());
}

Size SpriteListCtrl::GetItemSize(int index) const {
    return Size(GetSize().cx, 40); // Fixed height
}

void SpriteListCtrl::Paint(Draw& w) {
    if (!project) {
        w.DrawRect(GetSize(), White());
        return;
    }
    
    w.DrawRect(GetSize(), White());
    
    int y = 0;
    int item_height = 40;
    
    for (int i = 0; i < display_indices.GetCount(); i++) {
        Rect item_rect = RectC(0, y, GetSize().cx, item_height);
        DrawItem(w, i, item_rect);
        y += item_height;
    }
}

void SpriteListCtrl::MouseDown(Point pos, dword button) {
    int item_index = HitTest(pos);
    
    if (item_index >= 0) {
        selected_index = item_index;
        Refresh();
        
        // Call the selection callback
        if (select_callback) {
            const Sprite* selected_sprite = GetSelectedSprite();
            select_callback(selected_sprite);
        }
        
        // Start dragging if left button
        if (button == LEFT && selected_sprite) {
            is_dragging = true;
            drag_start = pos;
            
            // Start drag operation
            String drag_data = selected_sprite->id;
            ClipbdAction ca = DragAndDrop(this, drag_data, Image::Arrow());
        }
    } else {
        selected_index = -1;
        Refresh();
        
        if (select_callback) {
            select_callback(nullptr);
        }
    }
    
    Ctrl::MouseDown(pos, button);
}

void SpriteListCtrl::MouseMove(Point pos, dword keyflags) {
    if (is_dragging && (pos - drag_start).GetLength() > 5) { // Threshold to start drag
        if (selected_index >= 0 && project && selected_index < display_indices.GetCount()) {
            int project_index = display_indices[selected_index];
            const Sprite& sprite = project->sprites[project_index];
            
            String drag_data = sprite.id;
            ClipbdAction ca = DragAndDrop(this, drag_data, Image::Arrow());
            is_dragging = false; // Reset dragging state after starting drag
        }
    }
    
    Ctrl::MouseMove(pos, keyflags);
}

void SpriteListCtrl::RightDown(Point pos, dword flags) {
    int item_index = HitTest(pos);
    
    if (item_index >= 0) {
        // Select the item if it's not already selected
        if (selected_index != item_index) {
            selected_index = item_index;
            Refresh();
        }
        
        // Show context menu
        PopupContextMenu(pos);
    } else {
        // Deselect if clicking on empty space
        selected_index = -1;
        Refresh();
    }
    
    Ctrl::RightDown(pos, flags);
}

void SpriteListCtrl::PopupContextMenu(Point pos) {
    if (!project || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return;
    }
    
    int project_index = display_indices[selected_index];
    const Sprite& sprite = project->sprites[project_index];
    
    // Create context menu
    PopupWindow popup;
    popup.AddFrame(BlackFrame());
    
    // Create a menu layout
    WithTextCtrlLayout<ParentCtrl> content;
    content.Ctrl::SizeHint([this]() { return Size(150, 150); });
    
    // Create menu items
    Button edit_btn, duplicate_btn, delete_btn, cancel_btn;
    
    edit_btn.SetLabel("Edit Sprite");
    duplicate_btn.SetLabel("Duplicate Sprite");
    delete_btn.SetLabel("Delete Sprite");
    cancel_btn.SetLabel("Cancel");
    
    content.Add(edit_btn.TopPos(0, 24).HSizePos());
    content.Add(duplicate_btn.TopPos(28, 24).HSizePos());
    content.Add(delete_btn.TopPos(56, 24).HSizePos());
    content.Add(cancel_btn.TopPos(84, 24).HSizePos());
    
    // Define actions
    edit_btn <<= [this, project_index, &popup]() {
        // Open the edit dialog for the selected sprite
        if (!project || project_index >= project->sprites.GetCount()) {
            popup.Break();
            return;
        }
        
        Sprite& sprite = project->sprites[project_index];
        
        // Create dialog content using simple layout
        CtrlLayout<ParentCtrl> dlg;
        dlg.Ctrl::SizeHint([this]() { return Size(400, 320); });
        
        // Create input fields
        EditField id_field, name_field, texture_path_field, tags_field, description_field;
        SpinEdit region_x, region_y, region_cx, region_cy;
        SpinEdit pivot_x, pivot_y;
        Option category_option;
        Button ok_btn, cancel_btn, browse_btn;
        
        // Set up category options
        category_option.Add("character");
        category_option.Add("environment");
        category_option.Add("effect");
        category_option.Add("other");
        
        // Set up numeric fields
        region_x.SetRange(0, 10000);
        region_y.SetRange(0, 10000);
        region_cx.SetRange(1, 10000);
        region_cy.SetRange(1, 10000);
        pivot_x.SetRange(-1000, 1000);
        pivot_y.SetRange(-1000, 1000);
        
        // Initialize with current values
        id_field = sprite.id;
        name_field = sprite.name;
        texture_path_field = sprite.texture_path;
        
        // Set category
        int cat_index = category_option.Find(sprite.category);
        if (cat_index >= 0) {
            category_option.SetIndex(cat_index);
        } else {
            category_option.SetIndex(0); // Default to first option
        }
        
        // Set region values
        region_x.Set(sprite.region.x);
        region_y.Set(sprite.region.y);
        region_cx.Set(sprite.region.cx);
        region_cy.Set(sprite.region.cy);
        
        // Set pivot values
        pivot_x.Set(sprite.pivot.x);
        pivot_y.Set(sprite.pivot.y);
        
        // Set tags (join them with commas)
        String tags_text;
        for (int i = 0; i < sprite.tags.GetCount(); i++) {
            if (i > 0) tags_text += ", ";
            tags_text += sprite.tags[i];
        }
        tags_field = tags_text;
        
        description_field = sprite.description;
        
        // Add controls with positioning
        dlg.Add(id_field.HSizePos(80, 50).TopPos(8, 20));
        dlg.Add(name_field.HSizePos(80, 50).TopPos(32, 20));
        dlg.Add(category_option.HSizePos(80, 50).TopPos(56, 20));
        dlg.Add(texture_path_field.HSizePos(80, 25).TopPos(80, 20));
        dlg.Add(browse_btn.RightPos(8, 20).TopPos(80, 20));
        dlg.Add(region_x.RightPos(120, 25).TopPos(104, 20));
        dlg.Add(region_y.RightPos(85, 25).TopPos(104, 20));
        dlg.Add(region_cx.RightPos(45, 25).TopPos(104, 20));
        dlg.Add(region_cy.RightPos(5, 25).TopPos(104, 20));
        dlg.Add(pivot_x.RightPos(45, 25).TopPos(128, 20));
        dlg.Add(pivot_y.RightPos(5, 25).TopPos(128, 20));
        dlg.Add(tags_field.HSizePos(80, 50).TopPos(152, 20));
        dlg.Add(description_field.HSizePos(80, 50).TopPos(176, 60));
        dlg.Add(ok_btn.LeftPos(20, 60).BottomPos(8, 24));
        dlg.Add(cancel_btn.RightPos(20, 60).BottomPos(8, 24));
        
        // Labels
        Label id_label, name_label, category_label, texture_path_label, region_label, pivot_label, tags_label, description_label;
        id_label.SetLabel("ID:");
        name_label.SetLabel("Name:");
        category_label.SetLabel("Category:");
        texture_path_label.SetLabel("Texture Path:");
        region_label.SetLabel("Region (x,y,cx,cy):");
        pivot_label.SetLabel("Pivot (x,y):");
        tags_label.SetLabel("Tags:");
        description_label.SetLabel("Description:");
        
        dlg.Add(id_label.LeftPos(8, 60).TopPos(8, 20));
        dlg.Add(name_label.LeftPos(8, 60).TopPos(32, 20));
        dlg.Add(category_label.LeftPos(8, 60).TopPos(56, 20));
        dlg.Add(texture_path_label.LeftPos(8, 60).TopPos(80, 20));
        dlg.Add(region_label.LeftPos(8, 80).TopPos(104, 20));
        dlg.Add(pivot_label.LeftPos(8, 50).TopPos(128, 20));
        dlg.Add(tags_label.LeftPos(8, 60).TopPos(152, 20));
        dlg.Add(description_label.LeftPos(8, 60).TopPos(176, 20));
        
        browse_btn.SetLabel("...");
        browse_btn.SetTip("Browse for texture file");
        
        ok_btn.SetLabel("OK");
        cancel_btn.SetLabel("Cancel");
        
        // Browse button functionality
        browse_btn <<= [&]() {
            FileSel fs;
            fs.Type("Image Files (*.png, *.jpg, *.jpeg, *.bmp, *.tga)", "*.png;*.jpg;*.jpeg;*.bmp;*.tga");
            fs.Type("PNG Files", "*.png");
            fs.Type("JPG Files", "*.jpg;*.jpeg");
            fs.Type("BMP Files", "*.bmp");
            fs.Type("TGA Files", "*.tga");
            fs.AllFilesType();
            
            if (fs.ExecuteOpen("Select Texture File")) {
                String path = ~fs;
                texture_path_field = path;
                
                // Try to get image dimensions and set region accordingly
                FileIn in(path);
                if (in && in.IsOpen()) {
                    Image img = StreamRaster::LoadImage(in);
                    if (img) {
                        region_cx.Set(img.GetWidth());
                        region_cy.Set(img.GetHeight());
                    }
                }
            }
        };
        
        // Create dialog window
        PromptOKCancelFrame prompt_dlg;
        prompt_dlg.Title("Edit Sprite");
        prompt_dlg.Add(dlg.SizePos());
        prompt_dlg.OK(ok_btn);
        prompt_dlg.Cancel(cancel_btn);
        
        if(prompt_dlg.Execute() == IDOK) {
            // Update the sprite with new values
            String id = ~id_field;
            String name = ~name_field;
            String category = AsString(category_option.Get());
            String texture_path = ~texture_path_field;
            String tags_text = ~tags_field;
            String description_text = ~description_field;
            
            // Validate inputs
            if (id.IsEmpty()) {
                Exclamation("Sprite ID cannot be empty!");
                popup.Break();
                return;
            }
            
            // Check if ID is being changed and if new ID already exists
            if (id != sprite.id && project->FindSprite(id)) {
                Exclamation("A sprite with ID '" + id + "' already exists!");
                popup.Break();
                return;
            }
            
            // Update sprite properties
            sprite.id = id;
            sprite.name = name.IsEmpty() ? id : name;
            sprite.category = category;
            sprite.texture_path = texture_path;
            sprite.region = RectF(~region_x, ~region_y, ~region_cx, ~region_cy);
            sprite.pivot = Vec2(~pivot_x, ~pivot_y);
            
            // Parse tags - split by commas
            Vector<String> tokens = Split(tags_text, ',', true);
            sprite.tags.Clear();
            for (int i = 0; i < tokens.GetCount(); i++) {
                String tag = Trim(tokens[i]);
                if (!tag.IsEmpty()) {
                    sprite.tags.Add(tag);
                }
            }
            
            sprite.description = description_text;
            
            // Refresh the list to show the updated sprite
            RefreshList();
        }
        
        popup.Break();
    };
    
    duplicate_btn <<= [this, project_index, &popup]() {
        if (!project) return;
        
        // Create a duplicate of the selected sprite
        Sprite duplicated_sprite = project->sprites[project_index];
        
        // Generate a unique ID for the duplicate
        duplicated_sprite.id = GenerateUniqueIdForSprite(duplicated_sprite.id);
        duplicated_sprite.name = duplicated_sprite.name + "_copy";
        
        // Add the duplicated sprite to the project
        project->sprites.Add(duplicated_sprite);
        
        // Refresh the list to show the new sprite
        RefreshList();
        
        popup.Break();
    };
    
    delete_btn <<= [this, project_index, &popup, &sprite]() {
        if (!project) return;
        
        // Confirm deletion
        if (PromptYesNo("Are you sure you want to delete sprite '" + sprite.id + "'?")) {
            // Check if this sprite is used in any frames
            Vector<String> frames_using_sprite;
            for (int i = 0; i < project->frames.GetCount(); i++) {
                const Frame& frame = project->frames[i];
                for (int j = 0; j < frame.sprites.GetCount(); j++) {
                    if (frame.sprites[j].sprite_id == sprite.id) {
                        if (frames_using_sprite.Find(frame.id) == -1) {
                            frames_using_sprite.Add(frame.id);
                        }
                    }
                }
            }
            
            if (frames_using_sprite.GetCount() > 0) {
                String warning = "This sprite is used in " + IntStr(frames_using_sprite.GetCount()) + 
                                " frame(s). Deleting it will remove all references from those frames.\n\n" +
                                "Affected frames: ";
                for (int i = 0; i < frames_using_sprite.GetCount(); i++) {
                    warning += frames_using_sprite[i];
                    if (i < frames_using_sprite.GetCount() - 1) warning += ", ";
                }
                if (!PromptYesNo(warning + "\n\nAre you sure you want to continue?")) {
                    popup.Break();
                    return;
                }
                
                // Remove sprite references from all frames that use it
                for (int i = 0; i < project->frames.GetCount(); i++) {
                    Frame& frame = project->frames[i];
                    for (int j = frame.sprites.GetCount() - 1; j >= 0; j--) {
                        if (frame.sprites[j].sprite_id == sprite.id) {
                            frame.sprites.Remove(j);
                        }
                    }
                }
            }
            
            // Remove the sprite from the project
            project->sprites.Remove(project_index);
            
            // Refresh the list
            RefreshList();
        }
        popup.Break();
    };
    
    cancel_btn <<= [&popup]() {
        popup.Break();
    };
    
    // Set up the popup window
    popup.Add(content.SizePos());
    popup.SetRect(pos.x, pos.y, 150, 130);
    popup.Run();
}

String SpriteListCtrl::GenerateUniqueIdForSprite(const String& baseId) {
    if (!project) return baseId + "_1";
    
    // Extract the base name without suffix number
    String base = baseId;
    int suffix_num = 1;
    
    // If the ID already has a number suffix, separate them
    int underscore_pos = baseId.ReverseFind('_');
    if (underscore_pos != -1) {
        String suffix = baseId.Mid(underscore_pos + 1);
        if (IsDigit(suffix[0])) {
            base = baseId.Mid(0, underscore_pos);
            suffix_num = atoi(AsString(suffix));
        }
    }
    
    // Find an available number
    String newId;
    do {
        newId = base + "_" + IntStr(++suffix_num);
    } while (project->FindSprite(newId) != nullptr);
    
    return newId;
}

const Sprite* SpriteListCtrl::GetSelectedSprite() const {
    if (!project || selected_index < 0 || selected_index >= display_indices.GetCount()) {
        return nullptr;
    }
    
    int project_index = display_indices[selected_index];
    return &project->sprites[project_index];
}

void SpriteListCtrl::SetSortType(SortType sortType) {
    sort_type = sortType;
    ApplyFilters(); // Reapply filters which will cause resorting
}

void SpriteListCtrl::SortSprites() {
    switch (sort_type) {
        case SORT_BY_NAME:
            SortBy([](const Sprite& a, const Sprite& b) { 
                return ToLower(a.name) < ToLower(b.name); 
            });
            break;
        case SORT_BY_ID:
            SortBy([](const Sprite& a, const Sprite& b) { 
                return ToLower(a.id) < ToLower(b.id); 
            });
            break;
        case SORT_BY_CATEGORY:
            SortBy([](const Sprite& a, const Sprite& b) { 
                return ToLower(a.category) < ToLower(b.category); 
            });
            break;
        case SORT_BY_RECENT_USE:
        default:
            // For now, just maintain original order
            // In a real implementation, we'd sort by last used timestamp
            break;
    }
}

template<typename CompareFunc>
void SpriteListCtrl::SortBy(CompareFunc compareFunc) {
    // Sort the display_indices based on the comparison function
    for (int i = 0; i < display_indices.GetCount() - 1; i++) {
        for (int j = i + 1; j < display_indices.GetCount(); j++) {
            int idx1 = display_indices[i];
            int idx2 = display_indices[j];
            
            if (compareFunc(project->sprites[idx1], project->sprites[idx2])) {
                // Already in correct order
            } else {
                // Swap to sort in ascending order
                Upp::Swap(display_indices[i], display_indices[j]);
            }
        }
    }
}

String SpriteListCtrl::GenerateUniqueIdForSprite(const String& baseId) {
    if (!project) return baseId + "_1";
    
    // Extract the base name without suffix number
    String base = baseId;
    int suffix_num = 1;
    
    // If the ID already has a number suffix, separate them
    int underscore_pos = baseId.ReverseFind('_');
    if (underscore_pos != -1) {
        String suffix = baseId.Mid(underscore_pos + 1);
        if (IsDigit(suffix[0])) {
            base = baseId.Mid(0, underscore_pos);
            suffix_num = atoi(AsString(suffix));
        }
    }
    
    // Find an available number
    String newId;
    do {
        newId = base + "_" + IntStr(++suffix_num);
    } while (project->FindSprite(newId) != nullptr);
    
    return newId;
}