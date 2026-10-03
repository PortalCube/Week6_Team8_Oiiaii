#pragma once

class FEditor;

class IEditorWindow
{
public:
    virtual ~IEditorWindow() = default;
    virtual void Process(FEditor& Editor, float DeltaTime) = 0;
};