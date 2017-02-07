/*
 * CNVec2DEgo.h
 *
 *  Created on: 07.11.2016
 *      Author: Philipp Mandler
 */

#pragma once

#include "CNVec2DTemplate.h"

namespace geometry
{

class CNVec2DAllo;
class CNPositionAllo;
class CNVec2DAllo;

class CNVec2DEgo : public CNVec2DTemplate<CNVec2DEgo>
{
  public:
	CNVec2DEgo() : CNVec2DEgo(0, 0) {};
	CNVec2DEgo(double x, double y);
    virtual ~CNVec2DEgo();

    std::string toString();

    std::shared_ptr<CNVec2DAllo> toAllo(CNPositionAllo &origin);
};

} /* namespace geometry */