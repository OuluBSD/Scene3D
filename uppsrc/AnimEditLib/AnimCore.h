#ifndef _AnimEditLib_AnimCore_h_
#define _AnimEditLib_AnimCore_h_

#include <Core/Core.h>

namespace Upp {

static const int ANIMEDITLIB_VERSION_MAJOR = 0;
static const int ANIMEDITLIB_VERSION_MINOR = 1;

struct Vec2 : public Moveable<Vec2> {
    double x = 0.0, y = 0.0;
    Vec2() {}
    Vec2(double x, double y) : x(x), y(y) {}
    
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vec2& other) const { return !(*this == other); }
    
    // Arithmetic operators
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(double scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2 operator/(double scalar) const { return Vec2(x / scalar, y / scalar); }
    
    // Compound assignment operators
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(double scalar) { x *= scalar; y *= scalar; return *this; }
    Vec2& operator/=(double scalar) { x /= scalar; y /= scalar; return *this; }
    
    void Swap(Vec2& other) {
        Upp::Swap(x, other.x);
        Upp::Swap(y, other.y);
    }
};



struct RectF : public Moveable<RectF> {
    double x = 0.0, y = 0.0, cx = 0.0, cy = 0.0;
    RectF() {}
    RectF(double x, double y, double cx, double cy)
        : x(x), y(y), cx(cx), cy(cy) {}
    
    bool operator==(const RectF& other) const { 
        return x == other.x && y == other.y && cx == other.cx && cy == other.cy; 
    }
    bool operator!=(const RectF& other) const { return !(*this == other); }
    
    void Swap(RectF& other) {
        Upp::Swap(x, other.x);
        Upp::Swap(y, other.y);
        Upp::Swap(cx, other.cx);
        Upp::Swap(cy, other.cy);
    }
};

struct Sprite : public Moveable<Sprite> {
    String id;
    String name;
    String category;
    String texture_path;
    RectF  region;
    Vec2   pivot;
    Vector<String> tags;        // Metadata tags
    String description;         // Description of the sprite

    Sprite() = default;
    Sprite(const String& id)
        : id(id), name(id), category("default"), pivot(0,0) {}  // Default name to id
    
    bool operator==(const Sprite& other) const { 
        return id == other.id && name == other.name && category == other.category && 
               texture_path == other.texture_path && region == other.region && 
               pivot == other.pivot && tags == other.tags && description == other.description; 
    }
    bool operator!=(const Sprite& other) const { return !(*this == other); }
    
    void Swap(Sprite& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(category, other.category);
        Upp::Swap(texture_path, other.texture_path);
        Upp::Swap(region, other.region);
        Upp::Swap(pivot, other.pivot);
        Upp::Swap(tags, other.tags);
        Upp::Swap(description, other.description);
    }
};

struct SpriteInstance : public Moveable<SpriteInstance> {
    String sprite_id;
    Vec2   position;
    double rotation = 0;
    Vec2   scale = Vec2(1,1);
    double alpha = 1.0;
    int    zindex = 0;
    
    bool operator==(const SpriteInstance& other) const { 
        return sprite_id == other.sprite_id && position == other.position &&
               rotation == other.rotation && scale == other.scale &&
               alpha == other.alpha && zindex == other.zindex; 
    }
    bool operator!=(const SpriteInstance& other) const { return !(*this == other); }
    
    void Swap(SpriteInstance& other) {
        Upp::Swap(sprite_id, other.sprite_id);
        Upp::Swap(position, other.position);
        Upp::Swap(rotation, other.rotation);
        Upp::Swap(scale, other.scale);
        Upp::Swap(alpha, other.alpha);
        Upp::Swap(zindex, other.zindex);
    }
};

struct CollisionRect : public Moveable<CollisionRect> {
    String id;
    RectF  rect;
    
    bool operator==(const CollisionRect& other) const { 
        return id == other.id && rect == other.rect; 
    }
    bool operator!=(const CollisionRect& other) const { return !(*this == other); }
    
    void Swap(CollisionRect& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(rect, other.rect);
    }
};

struct AnimationFrame : public Moveable<AnimationFrame> {
    String id;
    String name;
    AnimationFrame() = default;
    AnimationFrame(const String& id) : id(id) {}

    Vector<SpriteInstance> sprites;
    Vector<CollisionRect>  collisions;
    double default_duration = 0.1;
    
    bool operator==(const AnimationFrame& other) const { 
        return id == other.id && name == other.name && 
               sprites == other.sprites && collisions == other.collisions &&
               default_duration == other.default_duration; 
    }
    bool operator!=(const AnimationFrame& other) const { return !(*this == other); }
    
    void Swap(AnimationFrame& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(sprites, other.sprites);
        Upp::Swap(collisions, other.collisions);
        Upp::Swap(default_duration, other.default_duration);
    }
};

struct AnimationFrameRef : public Moveable<AnimationFrameRef> {
    String frame_id;
    bool   has_duration = false;
    double duration = 0.0;
    
    bool operator==(const AnimationFrameRef& other) const { 
        return frame_id == other.frame_id && has_duration == other.has_duration &&
               duration == other.duration; 
    }
    bool operator!=(const AnimationFrameRef& other) const { return !(*this == other); }
    
    void Swap(AnimationFrameRef& other) {
        Upp::Swap(frame_id, other.frame_id);
        Upp::Swap(has_duration, other.has_duration);
        Upp::Swap(duration, other.duration);
    }
};

struct Animation : public Moveable<Animation> {
    String id;
    String name;
    String category;
    Vector<AnimationFrameRef> frames;
    
    bool operator==(const Animation& other) const { 
        return id == other.id && name == other.name && 
               category == other.category && frames == other.frames; 
    }
    bool operator!=(const Animation& other) const { return !(*this == other); }
    
    void Swap(Animation& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(category, other.category);
        Upp::Swap(frames, other.frames);
    }
};

struct AnimationTransition : public Moveable<AnimationTransition> {
    String from_animation_id;   // ID of the starting animation
    String to_animation_id;     // ID of the destination animation
    double transition_time;     // Time for the transition (in seconds)
    String condition;           // Condition for the transition (e.g., "speed > 0", "input.jump", etc.)
    String trigger_event;       // Event that triggers the transition

    AnimationTransition() : transition_time(0.2), condition("always") {}  // Default to 0.2s transition
    AnimationTransition(const String& from_id, const String& to_id)
        : from_animation_id(from_id), to_animation_id(to_id), transition_time(0.2), condition("always") {}

    bool operator==(const AnimationTransition& other) const {
        return from_animation_id == other.from_animation_id && to_animation_id == other.to_animation_id && 
               transition_time == other.transition_time && condition == other.condition && 
               trigger_event == other.trigger_event;
    }
    bool operator!=(const AnimationTransition& other) const { return !(*this == other); }

    void Swap(AnimationTransition& other) {
        Upp::Swap(from_animation_id, other.from_animation_id);
        Upp::Swap(to_animation_id, other.to_animation_id);
        Upp::Swap(transition_time, other.transition_time);
        Upp::Swap(condition, other.condition);
        Upp::Swap(trigger_event, other.trigger_event);
    }
};

struct AnimationEvent : public Moveable<AnimationEvent> {
    String id;
    String name;
    String type;           // Type of event (e.g., "sound", "particle", "callback", "trigger")
    int frame_index;       // AnimationFrame at which the event should trigger
    ValueMap parameters;   // Additional parameters specific to the event type

    AnimationEvent() : frame_index(0) {}
    AnimationEvent(const String& id, const String& name, const String& type, int frame_idx)
        : id(id), name(name), type(type), frame_index(frame_idx) {}

    bool operator==(const AnimationEvent& other) const {
        return id == other.id && name == other.name && type == other.type && 
               frame_index == other.frame_index && parameters == other.parameters;
    }
    bool operator!=(const AnimationEvent& other) const { return !(*this == other); }

    void Swap(AnimationEvent& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(type, other.type);
        Upp::Swap(frame_index, other.frame_index);
        Upp::Swap(parameters, other.parameters);
    }
};

struct AnimationBlendParams : public Moveable<AnimationBlendParams> {
    double weight = 1.0;     // Blend weight (0.0 to 1.0)
    double transition_time = 0.0;  // Transition time in seconds
    bool is_active = false;  // Whether this animation is currently active
    Vector<AnimationEvent> events; // Events that occur during this animation
    // Vector<ParameterAnimation> param_anims; // Parameter animations that occur during this animation - temporarily removed to fix compilation

    AnimationBlendParams() = default;
    
    bool operator==(const AnimationBlendParams& other) const {
        return weight == other.weight && transition_time == other.transition_time && 
               is_active == other.is_active && events == other.events;
               // && param_anims == other.param_anims;
    }
    bool operator!=(const AnimationBlendParams& other) const { return !(*this == other); }

    void Swap(AnimationBlendParams& other) {
        Upp::Swap(weight, other.weight);
        Upp::Swap(transition_time, other.transition_time);
        Upp::Swap(is_active, other.is_active);
        Upp::Swap(events, other.events);
        // Upp::Swap(param_anims, other.param_anims);
    }
};

// Keyframe for parameter animation
struct ParameterKeyframe : public Moveable<ParameterKeyframe> {
    double time;           // Time in seconds when this keyframe occurs
    double value;          // Parameter value at this keyframe
    String interpolation;  // Interpolation type: "linear", "ease_in", "ease_out", "bezier", etc.
    ValueMap metadata;     // Additional metadata for interpolation points
    
    ParameterKeyframe() : time(0.0), value(0.0), interpolation("linear") {}
    ParameterKeyframe(double time, double value) 
        : time(time), value(value), interpolation("linear") {}

    bool operator==(const ParameterKeyframe& other) const {
        return time == other.time && value == other.value &&
               interpolation == other.interpolation && metadata == other.metadata;
    }
    bool operator!=(const ParameterKeyframe& other) const { return !(*this == other); }

    void Swap(ParameterKeyframe& other) {
        Upp::Swap(time, other.time);
        Upp::Swap(value, other.value);
        Upp::Swap(interpolation, other.interpolation);
        Upp::Swap(metadata, other.metadata);
    }
};

// Parameter animation track - animates a single parameter over time
struct ParameterAnimation : public Moveable<ParameterAnimation> {
    String id;
    String name;
    String parameter_path;     // Path to the parameter being animated (e.g. "transform.scale.x", "material.color.r", etc.)
    String parameter_type;     // Type of the parameter ("float", "vec2", "vec3", "color", etc.)
    Vector<ParameterKeyframe> keyframes;  // Keyframes that define the animation
    bool loop;                 // Whether this parameter animation loops
    
    ParameterAnimation() : loop(false) {}
    ParameterAnimation(const String& id, const String& name, const String& param_path)
        : id(id), name(name), parameter_path(param_path), loop(false) {}

    bool operator==(const ParameterAnimation& other) const {
        return id == other.id && name == other.name && 
               parameter_path == other.parameter_path &&
               parameter_type == other.parameter_type &&
               keyframes == other.keyframes && loop == other.loop;
    }
    bool operator!=(const ParameterAnimation& other) const { return !(*this == other); }

    void Swap(ParameterAnimation& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(parameter_path, other.parameter_path);
        Upp::Swap(parameter_type, other.parameter_type);
        Upp::Swap(keyframes, other.keyframes);
        Upp::Swap(loop, other.loop);
    }
};

// Animation curve that can be applied to multiple parameters
struct AnimationCurve : public Moveable<AnimationCurve> {
    String id;
    String name;
    String curve_type;        // "linear", "bezier", "hermite", etc.
    Vector<Pointf> control_points;  // Control points for the curve
    double min_value;         // Minimum value of the curve
    double max_value;         // Maximum value of the curve
    bool is_looping;          // Whether the curve loops
    
    AnimationCurve() : min_value(0.0), max_value(1.0), is_looping(false) {}
    AnimationCurve(const String& id, const String& name) 
        : id(id), name(name), min_value(0.0), max_value(1.0), is_looping(false) {}

    bool operator==(const AnimationCurve& other) const {
        return id == other.id && name == other.name && 
               curve_type == other.curve_type &&
               control_points == other.control_points &&
               min_value == other.min_value && 
               max_value == other.max_value && 
               is_looping == other.is_looping;
    }
    bool operator!=(const AnimationCurve& other) const { return !(*this == other); }

    void Swap(AnimationCurve& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(curve_type, other.curve_type);
        Upp::Swap(control_points, other.control_points);
        Upp::Swap(min_value, other.min_value);
        Upp::Swap(max_value, other.max_value);
        Upp::Swap(is_looping, other.is_looping);
    }
};

// Trigger region in the animation space
struct TriggerRegion : public Moveable<TriggerRegion> {
    String id;
    String name;
    String type;             // "box", "circle", "polygon", "point" etc.
    RectF bounds;            // Bounding area of the region
    ValueMap properties;     // Additional properties like tags, groups, etc.
    Vector<String> responses; // IDs of responses to trigger
    bool is_active;          // Whether this trigger is currently active
    
    TriggerRegion() : type("box"), is_active(true) {}
    TriggerRegion(const String& id, const String& name, const RectF& bounds)
        : id(id), name(name), type("box"), bounds(bounds), is_active(true) {}

    bool operator==(const TriggerRegion& other) const {
        return id == other.id && name == other.name && type == other.type &&
               bounds == other.bounds && properties == other.properties &&
               responses == other.responses && is_active == other.is_active;
    }
    bool operator!=(const TriggerRegion& other) const { return !(*this == other); }

    void Swap(TriggerRegion& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(type, other.type);
        Upp::Swap(bounds, other.bounds);
        Upp::Swap(properties, other.properties);
        Upp::Swap(responses, other.responses);
        Upp::Swap(is_active, other.is_active);
    }
};

// Response to a trigger event
struct TriggerResponse : public Moveable<TriggerResponse> {
    String id;
    String name;
    String type;             // "animation", "script", "event", "state_change", etc.
    String target_id;        // ID of target entity/animation to affect
    String action;           // Specific action to take
    ValueMap parameters;     // Parameters for the action
    
    TriggerResponse() : type("event") {}
    TriggerResponse(const String& id, const String& name, const String& type)
        : id(id), name(name), type(type) {}

    bool operator==(const TriggerResponse& other) const {
        return id == other.id && name == other.name && type == other.type &&
               target_id == other.target_id && action == other.action &&
               parameters == other.parameters;
    }
    bool operator!=(const TriggerResponse& other) const { return !(*this == other); }

    void Swap(TriggerResponse& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(type, other.type);
        Upp::Swap(target_id, other.target_id);
        Upp::Swap(action, other.action);
        Upp::Swap(parameters, other.parameters);
    }
};

// Trigger system that can be attached to entities or animations
struct TriggerSystem : public Moveable<TriggerSystem> {
    String id;
    String name;
    Vector<TriggerRegion> regions;    // Regions that can trigger responses
    Vector<TriggerResponse> responses; // Responses that can be triggered
    ValueMap properties;             // Global properties for the trigger system
    
    TriggerSystem() {}
    TriggerSystem(const String& id, const String& name)
        : id(id), name(name) {}

    bool operator==(const TriggerSystem& other) const {
        return id == other.id && name == other.name &&
               regions == other.regions && responses == other.responses &&
               properties == other.properties;
    }
    bool operator!=(const TriggerSystem& other) const { return !(*this == other); }

    void Swap(TriggerSystem& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(regions, other.regions);
        Upp::Swap(responses, other.responses);
        Upp::Swap(properties, other.properties);
    }
};

struct NamedAnimationSlot : public Moveable<NamedAnimationSlot> {
    String name;
    String animation_id;
    AnimationBlendParams blend_params;  // Parameters for animation blending

    NamedAnimationSlot() = default;
    NamedAnimationSlot(const String& name, const String& animation_id)
        : name(name), animation_id(animation_id) {}

    bool operator==(const NamedAnimationSlot& other) const {
        return name == other.name && animation_id == other.animation_id && blend_params == other.blend_params;
    }
    bool operator!=(const NamedAnimationSlot& other) const { return !(*this == other); }

    void Swap(NamedAnimationSlot& other) {
        Upp::Swap(name, other.name);
        Upp::Swap(animation_id, other.animation_id);
        Upp::Swap(blend_params, other.blend_params);
    }
};

struct EntityAnimationParams : public Moveable<EntityAnimationParams> {
    double speed_multiplier = 1.0;  // Multiplier for animation playback speed (1.0 = normal speed)
    double time_offset = 0.0;       // Time offset in seconds to start animation from a different point
    bool is_looping = true;         // Whether animations should loop by default for this entity

    EntityAnimationParams() = default;
    
    bool operator==(const EntityAnimationParams& other) const {
        return speed_multiplier == other.speed_multiplier && 
               time_offset == other.time_offset && 
               is_looping == other.is_looping;
    }
    bool operator!=(const EntityAnimationParams& other) const { return !(*this == other); }

    void Swap(EntityAnimationParams& other) {
        Upp::Swap(speed_multiplier, other.speed_multiplier);
        Upp::Swap(time_offset, other.time_offset);
        Upp::Swap(is_looping, other.is_looping);
    }
};

struct EntityScript : public Moveable<EntityScript> {
    String id;
    String name;
    String type;           // "lua", "javascript", "expression", etc.
    String content;        // The actual script content
    ValueMap parameters;   // Parameters/variables for the script
    bool is_active;        // Whether this script should be executed

    EntityScript() : is_active(true) {}
    EntityScript(const String& id, const String& name, const String& type, const String& content)
        : id(id), name(name), type(type), content(content), is_active(true) {}

    bool operator==(const EntityScript& other) const {
        return id == other.id && name == other.name && type == other.type &&
               content == other.content && parameters == other.parameters &&
               is_active == other.is_active;
    }
    bool operator!=(const EntityScript& other) const { return !(*this == other); }

    void Swap(EntityScript& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(type, other.type);
        Upp::Swap(content, other.content);
        Upp::Swap(parameters, other.parameters);
        Upp::Swap(is_active, other.is_active);
    }
};

// Behavior tree node types
enum class BTNodeType {
    ACTION,
    CONDITION,
    SEQUENCE,
    SELECTOR,
    DECORATOR,
    ROOT
};

// Base behavior tree node
struct BTNode : public Moveable<BTNode> {
    String id;
    BTNodeType node_type;
    String name;
    String type_name;          // Specific type like "MoveTo", "IsHealthLow", etc.
    ValueMap parameters;       // Parameters for this node
    Vector<String> children;   // IDs of child nodes
    String parent;             // ID of parent node (empty if root)
    
    BTNode() : node_type(BTNodeType::ACTION) {}
    BTNode(const String& id, BTNodeType type, const String& name)
        : id(id), node_type(type), name(name) {}

    bool operator==(const BTNode& other) const {
        return id == other.id && node_type == other.node_type &&
               name == other.name && type_name == other.type_name &&
               parameters == other.parameters &&
               children == other.children && parent == other.parent;
    }
    bool operator!=(const BTNode& other) const { return !(*this == other); }

    void Swap(BTNode& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(node_type, other.node_type);
        Upp::Swap(name, other.name);
        Upp::Swap(type_name, other.type_name);
        Upp::Swap(parameters, other.parameters);
        Upp::Swap(children, other.children);
        Upp::Swap(parent, other.parent);
    }
};

// Behavior tree structure
struct BehaviorTree : public Moveable<BehaviorTree> {
    String id;
    String name;
    Vector<BTNode> nodes;      // All nodes in the tree
    String root_node_id;       // ID of the root node
    
    BehaviorTree() {}
    BehaviorTree(const String& id, const String& name)
        : id(id), name(name), root_node_id("") {}

    bool operator==(const BehaviorTree& other) const {
        return id == other.id && name == other.name &&
               nodes == other.nodes && root_node_id == other.root_node_id;
    }
    bool operator!=(const BehaviorTree& other) const { return !(*this == other); }

    void Swap(BehaviorTree& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(nodes, other.nodes);
        Upp::Swap(root_node_id, other.root_node_id);
    }
};

// State for state machine
struct StateNode : public Moveable<StateNode> {
    String id;
    String name;
    String type;               // Type of state like "Idle", "Walking", "Attacking", etc.
    ValueMap parameters;       // Parameters for this state
    Vector<String> transitions; // IDs of transition nodes from this state
    
    StateNode() {}
    StateNode(const String& id, const String& name, const String& type)
        : id(id), name(name), type(type) {}

    bool operator==(const StateNode& other) const {
        return id == other.id && name == other.name && type == other.type &&
               parameters == other.parameters && transitions == other.transitions;
    }
    bool operator!=(const StateNode& other) const { return !(*this == other); }

    void Swap(StateNode& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(type, other.type);
        Upp::Swap(parameters, other.parameters);
        Upp::Swap(transitions, other.transitions);
    }
};

// Transition for state machine
struct StateTransition : public Moveable<StateTransition> {
    String id;
    String from_state;         // ID of the source state
    String to_state;           // ID of the target state
    String condition;          // Condition that triggers this transition
    String action;             // Action to perform during transition
    ValueMap parameters;       // Parameters for this transition
    
    StateTransition() {}
    StateTransition(const String& id, const String& from, const String& to)
        : id(id), from_state(from), to_state(to) {}

    bool operator==(const StateTransition& other) const {
        return id == other.id && from_state == other.from_state &&
               to_state == other.to_state && condition == other.condition &&
               action == other.action && parameters == other.parameters;
    }
    bool operator!=(const StateTransition& other) const { return !(*this == other); }

    void Swap(StateTransition& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(from_state, other.from_state);
        Upp::Swap(to_state, other.to_state);
        Upp::Swap(condition, other.condition);
        Upp::Swap(action, other.action);
        Upp::Swap(parameters, other.parameters);
    }
};

// State machine structure
struct StateMachine : public Moveable<StateMachine> {
    String id;
    String name;
    Vector<StateNode> states;
    Vector<StateTransition> transitions;
    String initial_state_id;   // ID of the initial state
    
    StateMachine() {}
    StateMachine(const String& id, const String& name)
        : id(id), name(name), initial_state_id("") {}

    bool operator==(const StateMachine& other) const {
        return id == other.id && name == other.name &&
               states == other.states && transitions == other.transitions &&
               initial_state_id == other.initial_state_id;
    }
    bool operator!=(const StateMachine& other) const { return !(*this == other); }

    void Swap(StateMachine& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(states, other.states);
        Upp::Swap(transitions, other.transitions);
        Upp::Swap(initial_state_id, other.initial_state_id);
    }
};

struct Entity : public Moveable<Entity> {
    String id;
    String name;
    String type;
    Vector<NamedAnimationSlot> animation_slots;
    Vector<AnimationTransition> animation_transitions;  // Transitions between animation states
    EntityAnimationParams anim_params;                  // Animation parameters specific to this entity
    ValueMap properties;  // Simple key-value store for behavior parameters
    Vector<EntityScript> scripts;  // Scripts for entity behaviors
    Vector<BehaviorTree> behavior_trees;  // Behavior trees for entity AI
    Vector<StateMachine> state_machines;  // State machines for entity AI
    Vector<TriggerSystem> trigger_systems;  // Trigger systems for entity interactions
    String parent_id;                       // ID of parent entity (empty if root)
    Vector<String> children_ids;            // IDs of child entities
    bool inherit_transform;                 // Whether to inherit parent's transform

    Entity() = default;
    Entity(const String& id)
        : id(id), name(id), type("default"), inherit_transform(true) {}

    bool operator==(const Entity& other) const {
        return id == other.id && name == other.name && type == other.type &&
               animation_slots == other.animation_slots &&
               animation_transitions == other.animation_transitions &&
               anim_params == other.anim_params &&
               properties == other.properties &&
               scripts == other.scripts &&
               behavior_trees == other.behavior_trees &&
               state_machines == other.state_machines &&
               trigger_systems == other.trigger_systems &&
               parent_id == other.parent_id && 
               children_ids == other.children_ids &&
               inherit_transform == other.inherit_transform;
    }
    bool operator!=(const Entity& other) const { return !(*this == other); }

    void Swap(Entity& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(type, other.type);
        Upp::Swap(animation_slots, other.animation_slots);
        Upp::Swap(animation_transitions, other.animation_transitions);
        Upp::Swap(anim_params, other.anim_params);
        Upp::Swap(properties, other.properties);
        Upp::Swap(scripts, other.scripts);
        Upp::Swap(behavior_trees, other.behavior_trees);
        Upp::Swap(state_machines, other.state_machines);
        Upp::Swap(trigger_systems, other.trigger_systems);
        Upp::Swap(parent_id, other.parent_id);
        Upp::Swap(children_ids, other.children_ids);
        Upp::Swap(inherit_transform, other.inherit_transform);
    }
};

struct AnimationProject : public Moveable<AnimationProject> {
    String id;
    String name;

    Vector<Sprite>     sprites;
    Vector<AnimationFrame>      frames;
    Vector<Animation>  animations;
    Vector<Entity>     entities;
    Vector<BehaviorTree> behavior_trees;  // Shared behavior trees across entities
    Vector<StateMachine> state_machines;  // Shared state machines across entities
    Vector<AnimationCurve> curves;        // Shared animation curves across entities
    Vector<TriggerSystem> trigger_systems; // Shared trigger systems across entities
    Vector<String> entity_groups;          // Groups of entities for collective operations

    const Sprite*    FindSprite(const String&) const;
    Sprite*          FindSprite(const String&);

    const AnimationFrame*     FindFrame(const String&) const;
    AnimationFrame*           FindFrame(const String&);

    const Animation* FindAnimation(const String&) const;
    Animation*       FindAnimation(const String&);

    const Entity*    FindEntity(const String&) const;
    Entity*          FindEntity(const String&);

    const BehaviorTree* FindBehaviorTree(const String&) const;
    BehaviorTree*       FindBehaviorTree(const String&);

    const StateMachine* FindStateMachine(const String&) const;
    StateMachine*       FindStateMachine(const String&);

    const AnimationCurve* FindCurve(const String&) const;
    AnimationCurve*       FindCurve(const String&);

    const TriggerSystem* FindTriggerSystem(const String&) const;
    TriggerSystem*       FindTriggerSystem(const String&);

    bool operator==(const AnimationProject& other) const { 
        return id == other.id && name == other.name && 
               sprites == other.sprites && frames == other.frames &&
               animations == other.animations && entities == other.entities &&
               behavior_trees == other.behavior_trees && state_machines == other.state_machines &&
               curves == other.curves && trigger_systems == other.trigger_systems &&
               entity_groups == other.entity_groups; 
    }
    bool operator!=(const AnimationProject& other) const { return !(*this == other); }
    
    void Swap(AnimationProject& other) {
        Upp::Swap(id, other.id);
        Upp::Swap(name, other.name);
        Upp::Swap(sprites, other.sprites);
        Upp::Swap(frames, other.frames);
        Upp::Swap(animations, other.animations);
        Upp::Swap(entities, other.entities);
        Upp::Swap(behavior_trees, other.behavior_trees);
        Upp::Swap(state_machines, other.state_machines);
        Upp::Swap(curves, other.curves);
        Upp::Swap(trigger_systems, other.trigger_systems);
        Upp::Swap(entity_groups, other.entity_groups);
    }
};

} // namespace Upp

#endif