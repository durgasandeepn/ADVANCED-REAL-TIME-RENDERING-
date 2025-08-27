#pragma once

class CreateTexture {

public:

	CreateTexture();//constructor

	void CreatingTexture(int Width, int Height, unsigned int* TextureId);
    void BindTexture(const int unit, const int programId, const std::string& name, GLint TextureID);
};

