#include "DERendering/DERendering.h"
#include "FramegraphBuilder.h"
#include "Framegraph/FramegraphPassBase.h"
#include "Framegraph.h"

namespace DE
{

FramegraphResource* FramegraphBuilder::Read(FramegraphResource* prev)
{
	FramegraphResource* resource = new FramegraphResource(FramegraphResource::State::READ, prev);
	m_pFramegraph->m_resources.push_back(resource);

	m_pPass->m_drawResources.push_back(resource);
	return resource;
}

FramegraphResource* FramegraphBuilder::Write(FramegraphResource* prev)
{
	FramegraphResource* resource = new FramegraphResource(FramegraphResource::State::WRITE, prev);
	m_pFramegraph->m_resources.push_back(resource);

	m_pPass->m_writeResources.push_back(resource);
	return resource;
}

FramegraphResource* FramegraphBuilder::Draw(FramegraphResource* prev)
{
	FramegraphResource* resource = new FramegraphResource(FramegraphResource::State::DRAW, prev);
	m_pFramegraph->m_resources.push_back(resource);

	m_pPass->m_drawResources.push_back(resource);
	return resource;
}

}
