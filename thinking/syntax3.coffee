# reference https://doc.qt.io/qt-6/qtqml-syntax-objectattributes.html

# a comment

# Declaration with children
Control {
    # Declaration without children as a child
    Control {}
}

# Declaration without children at root level
Control {}

# Declaration with properties
Control {
    # Property definition
    # string
    prop_name: "value"
    # integer number
    name: 123
    # float number
    name: 123.123
    # boolean
    name: true
}

# example of a score bar

MarginContainer {
    anchor_left: 0.15
    anchor_top: 0.03
    anchor_right: 0.85
    anchor_bottom: 0.08
    grow_horizontal: GrowDirection.Both

    ProgressBar {
        show_percentage: false
        size_flags_vertical: SizeFlags.SIZE_FILL
        theme.background: StyleBoxFlat {
            bg_color: "#b7b7b7"
        }
        theme.fill: StyleBoxFlat {
            bg_color: "#ffff00"
        }

        Label {
            horizontal_alignment: HorizontalAlignment.HORIZONTAL_ALIGNMENT_CENTER
            vertical_alignment: VerticalAlignment.VERTICAL_ALIGNMENT_CENTER
            text: "0"
            label_settings: LabelSettings {
                font_size: 32
                font_color: "#000000"
            }
        }
    }
}