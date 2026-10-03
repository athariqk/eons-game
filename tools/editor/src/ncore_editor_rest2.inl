    // TODO: refactor this to use Timers
    scene.get_ecs()
        .system( "EngineEditorPlugin_TitleBarUpdater" )
        .with<WindowComponent>()
        .in( EcsSystemPhase::POST_FRAME )
        .each( []( EcsIterState& it ) {
            // auto vid  = it.world().get_singleton<VideoServices>();
            // auto time = it.world().get_singleton<TimeComponent>();

            // auto window = it.get_component<WindowComponent>();
            // if (time->accumulator >= 0.5) {
            //     update_window_title( vid->window, it.entity(), window, time->fps, it.delta_time() );
            // }
        } );

    scene.get_ecs()
        .system( "EngineEditorPlugin_InputUI" )
        .with<IOServices>()
        .in( EcsSystemPhase::UPDATE )
        .run( []( EcsIterState& it ) {
            auto state = it.world().get_singleton<EditorState>();
            if (!state->ShowInputsWindow)
                return;

            auto io = it.world().get_singleton<IOServices>();

            if (ImGui::Begin( "Input Debug", &state->ShowInputsWindow )) {
                {
                    ImGui::SeparatorText( "Actions" );
                    auto actions = io->Inputs->action_list();

                    if (ImGui::BeginTable( "ActionsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg )) {
                        ImGui::TableSetupColumn( "Name" );
                        ImGui::TableSetupColumn( "Held" );
                        ImGui::TableSetupColumn( "Pressed" );
                        ImGui::TableSetupColumn( "Released" );
                        ImGui::TableHeadersRow();

                        for (const auto& action : actions) {
                            bool held     = io->Inputs->action_is_held( action.data() );
                            bool pressed  = io->Inputs->action_is_pressed( action.data() );
                            bool released = io->Inputs->action_is_released( action.data() );

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex( 0 );
                            ImGui::Text( "%s", action.data() );
                            ImGui::TableSetColumnIndex( 1 );
                            ImGui::TextColored(
                                held ? ImVec4( 0.4f, 1.0f, 0.4f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                held ? "Yes" : "No"
                            );
                            ImGui::TableSetColumnIndex( 2 );
                            ImGui::TextColored(
                                pressed ? ImVec4( 1.0f, 1.0f, 0.3f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                pressed ? "Yes" : "No"
                            );
                            ImGui::TableSetColumnIndex( 3 );
                            ImGui::TextColored(
                                released ? ImVec4( 1.0f, 0.6f, 0.2f, 1.0f ) : ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
                                released ? "Yes" : "No"
                            );
                        }
                        ImGui::EndTable();
                    }
                }

                {
                    ImGui::SeparatorText( "Mouse Input" );
                    ImGui::DragFloat2(
                        "Pos", io->Inputs->get_mouse_position().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Delta", io->Inputs->get_mouse_delta().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                    ImGui::DragFloat2(
                        "Wheel", io->Inputs->get_mouse_wheel().data(), 1.0f, 0.0f, 0.0f, "%.3f",
                        ImGuiSliderFlags_NoInput
                    );
                }
            }
            ImGui::End();
        } );

    scene.get_ecs()
        .observer( "EngineEditorPlugin_Cleanup" )
        .on<EditorState>( EcsCoreEvent::OnRemove )
        .run( []( EcsIterState& it ) {
            auto state = it.world().get_singleton<EditorState>();
            auto vid   = it.world().get_singleton<VideoServices>();
            if (state && vid) {
                if (state->ViewportRT)
                    vid->Renderer->destroy_rid( state->ViewportRT );
                if (state->ViewportDT)
                    vid->Renderer->destroy_rid( state->ViewportDT );
                if (state->GameViewRT)
                    vid->Renderer->destroy_rid( state->GameViewRT );
                if (state->GameViewDT)
                    vid->Renderer->destroy_rid( state->GameViewDT );
            }
        } );
}

void NCAPI_EDITOR unregister_editor_plugin( Scene& scene )
{
    scene.get_ecs().remove_singleton<EditorState>();
    unregister_gui_plugin( scene );
}

} // namespace nc::editor
