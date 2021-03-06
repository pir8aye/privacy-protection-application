/*******************************************************************************
 * Copyright 2018 UT-Battelle, LLC
 * All rights reserved
 * Route Sanitizer, version 0.9
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For issues, question, and comments, please submit a issue via GitHub.
 *******************************************************************************/
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// NOTE: The test file OPERAND is a <test-spec> (see github.com/philsquared/Catch/blob/master/docs/command-line.md) 
// <test-spec> is defined as below.
// NOTE: Tests can be hidden by starting the tag with a '.' character.
// NOTE: Test specifiers are insensitive.
// NOTE: Test specifiers can include * as a wildcard.
// NOTE: Test specifiers can include ~ as a negation operator, e.g., ~testa = all test cases except testa.
// NOTE: If test specifier includes spaces, quote the specifier on the CL.
// NOTE: specifiers in square brackets can be used to develop predicates: [one][two],[three].  All tests tagged with one AND two OR tagged with three.

#include <memory>
#include <bitset>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
// #include <iterator>
// #include <algorithm>
#include <regex>

#include "cvlib.hpp"

Quad::Ptr buildTestQuadTree( void ) {
    geo::Point sw{ 35.946920, -83.938486 };
    geo::Point ne{ 35.955526, -83.926738 };
    Quad::Ptr qptr = std::make_shared<Quad>(sw, ne);

    shapes::CSVInputFactory shape_factory("unit-test-data/lib-test-data/utk.quad");
    shape_factory.make_shapes(); 

    for (auto& edge_ptr : shape_factory.get_edges()) {
        Quad::insert(qptr, std::dynamic_pointer_cast<const geo::Entity>(edge_ptr)); 
    }

    return qptr;
}

TEST_CASE( "Parse Shape File Data", "[quad][shapefile]" ) {

    // Edge Specification:
    // - line_parts[0] : "edge"
    // - line_parts[1] : unique 64-bit integer identifier
    // - line_parts[2] : A sequence of two colon-split points; 
    //      - each point is semi-colon split.
    //      - Point: <uid>;latitude;longitude
    // - line_parts[3] : A sequence of colon-split key=value attributes.
    //      - Attribute Pair: <attribute>=<value>
    //
    // Circle Specification:
    // - line_parts[0] : "circle"
    // - line_parts[1] : unique 64-bit integer identifier
    // - line_parts[2] : A sequence of colon-split elements that define the center.
    //      - Center: <lat>:<lon>:<radius in meters>
    // 
    // Grid Specification:
    // - line_parts[0] : "grid"
    // - line_parts[1] : A '_' split row-column pair.
    // - line_parts[2] : A sequence of 4 colon-split elements defining the grid position.
    //      - Point: <sw lat>:<sw lon>:<ne lat>:<ne lon>
    //
    

    std::vector<std::string> argnum_tests {
        "",
        "edge, 11",
        // too many edge points or not enough points to define edge.
        "edge, 12, 0;0;0:1;1;1:2;2;2",
        "edge, 13, 0;0;0:1;1",
        "edge, 14, 0;0 : 1;1;1"
    };

    std::vector<std::string> argnum_grid_tests {
        "",
        "grid,0_0,-83.91:42.431661:-83.89782906874559",
        "grid,0_0,42.431661:-83.89782906874559",
        "grid,0_0,-83.89782906874559"
    };

    std::vector<std::string> argnum_circle_tests {
        "",
        "circle,0,-83.735670:22.0",
        "circle,0,22.0"
    };

    // data type problems.
    std::vector<std::string> datatype_tests {
        "edge, X , 3;0;0 : 4;1;1",
        "edge, 21, X;0;0 : 5;1;1",
        "edge, 22, 6;0;0 : X;1;1",
        "edge, 23, 7;a;- : 8;1;1",
        "edge, 24, 9;0;0 : 10;x;*"
    };

    std::vector<std::string> datatype_grid_tests {
        "grid,X,42.42267784715881:-83.91:42.431661:-83.89782906874559",
        "grid,0_0,X:-83.91:42.431661:-83.89782906874559",
        "grid,0_0,42.42267784715881:X:42.431661:-83.89782906874559"
        "grid,0_0,42.42267784715881:-83.91:X:-83.89782906874559",
        "grid,0_0,42.42267784715881:-83.91:42.431661:X"
    };

    std::vector<std::string> datatype_circle_tests {
        "circle,X,42.283135:-83.735670:22.0",
        "circle,0,X:-83.735670:22.0",
        "circle,0,42.283135:X:22.0",
        "circle,0,42.283135:-83.735670:X"
    };

    // out of lat/lon range problems; in a loop so one point check is sufficient.
    std::vector<std::string> badposition_tests {
        "edge,31, 11; 80.1;0       :15;1;1",
        "edge,32, 12;-84.1;0       :16;1;1",
        "edge,33, 13; 0    ; 180.1 :17;1;1",
        "edge,34, 14; 0    ;-180.1 :18;1;1"
    };

    std::vector<std::string> badposition_grid_tests {
        "grid,0_0,80.1:0:1:1",
        "grid,0_0,-84.1:0:1:1",
        "grid,0_0,0:180.1:1:1",
        "grid,0_0,0:-180.1:1:1"
        "grid,0_0,0:0,80.1:0:",
        "grid,0_0,0:0:-84.1:0",
        "grid,0_0,0:0:0:180",
        "grid,0_0,0:0:0:-180"
    };

    std::vector<std::string> badposition_circle_tests {
        "circle,0,80.1:0:22.0",
        "circle,0,-84.1:0:22.0",
        "circle,0,0:180:22.0",
        "circle,0,0:-180:22.0",
        "circle,0,42.283135:-83.735670:-22.0"
    };

    std::vector<std::string> badedge_tests {
        // same id for points.
        "edge,41, 19;0;0       :19;0;0"
    };

    // make sure SERVICE is in there for testing the invalid_way_exception
    osm::highway_blacklist.insert( osm::Highway::SERVICE );
    // this also tests strip and caps business.
    std::vector<std::string> waytype_tests {
        "edge,58, 31 ; 41.24 ; -83.74 : 61 ; 41.25 ; -84.04 , way_type = SERVICE",
        "edge,59, 31 ; 41.24 ; -83.74 : 62 ; 41.25 ; -84.04 , way_type = servicE",
        "edge,60, 31 ; 41.24 ; -83.74 : 63 ; 41.25 ; -84.04 , way_type = service"
    };

    std::vector<std::string> good_grid_tests {
        "grid,0_0,42.42267784715881:-83.91:42.431661:-83.89782906874559",
        "grid,0_1,42.42267784715881:-83.89782906874559:42.431661:-83.88565813749122",
        "grid,0_2,42.42267784715881:-83.88565813749122:42.431661:-83.87348720623683",
        "grid,0_3,42.42267784715881:-83.87348720623683:42.431661:-83.86131627498244"
    };

    std::vector<std::string> good_circle_tests {
        "circle,0,42.283135:-83.735670:22.0",
        "circle,1,42.297902:-83.720502:32.0",
        "circle,2,42.304978:-83.692901:32.0",
        "circle,3,42.302505:-83.707290:22.0"
    };

    std::vector<std::string> good_tests {
       "edge,71, 51 ; 41.1 ; -83.1 : 52 ; 41.2 ; -84.2 , way_type = primary : way_id=80",
       "edge,73, 53 ; 41.3 ; -83.3 : 54 ; 41.4 ; -84.4 , way_type = primary : way_id=80",
       "edge,75, 55 ; 41.5 ; -83.5 : 56 ; 41.6 ; -84.6 , way_type = primary : way_id=80"
    };

    // checks that we pull from a previously defined vertices.
    std::vector<std::string> bad_tests {
       "edge,77, 55 ; 41.7 ; -83.7 : 56 ; 41.8 ; -84.8 , way_type = primary : way_id=80"
    };


    shapes::CSVInputFactory sf{};

    for ( auto& testline : argnum_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_edge( parts ), std::logic_error );
    }

    for ( auto& testline : datatype_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_edge( parts ), std::logic_error );
    }

    for ( auto& testline : badposition_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_edge( parts ), std::logic_error );
    }

    for ( auto& testline : badedge_tests  ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_edge( parts ), std::logic_error );
    }

    for ( auto& testline : argnum_grid_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_grid( parts ), std::logic_error );
    }

    for ( auto& testline : argnum_circle_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_circle( parts ), std::logic_error );
    }

    for ( auto& testline : datatype_grid_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_grid( parts ), std::logic_error );
    }

    for ( auto& testline : datatype_grid_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_circle( parts ), std::logic_error );
    }

    for ( auto& testline : badposition_grid_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_grid( parts ), std::logic_error );
    }

    for ( auto& testline : badposition_circle_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_THROWS_AS( sf.make_circle( parts ), std::logic_error );
    }

    for ( auto& testline : good_grid_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_NOTHROW( sf.make_grid( parts ) );
    }

    for ( auto& testline : good_circle_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        CHECK_NOTHROW( sf.make_circle( parts ) );
    }

    int i = 1;
    for ( auto& testline : good_tests  ) {
        StrVector parts = string_utilities::split(testline, ',');
        // check lat/lon range.
        CHECK_NOTHROW( sf.make_edge( parts ) );
        CHECK( sf.get_edges().back()->get_uid() == 70+i  );

        CHECK( sf.get_edges().back()->get_way_type() == osm::Highway::PRIMARY  );

        CHECK( sf.get_edges().back()->v1->uid == 50+i );
        CHECK( sf.get_edges().back()->v2->uid == 51+i );

        CHECK( sf.get_edges().back()->v1->lat == Approx(41.0 + (i/10.0)) );
        CHECK( sf.get_edges().back()->v2->lat == Approx(41.1 + (i/10.0)) );

        CHECK( sf.get_edges().back()->v1->lon == Approx(-83.0 - (i/10.0)) );
        CHECK( sf.get_edges().back()->v2->lon == Approx(-84.1 - (i/10.0)) );

        i+=2; 
    }

    i = 7;
    for ( auto& testline : bad_tests ) {
        StrVector parts = string_utilities::split(testline, ',');
        // check lat/lon range.
        CHECK_NOTHROW( sf.make_edge( parts ) );
        CHECK_FALSE( sf.get_edges().back()->v1->uid == 50+i );
        CHECK_FALSE( sf.get_edges().back()->v2->uid == 51+i );
        CHECK_FALSE( sf.get_edges().back()->v1->lat == Approx(41.0 + (i/10.0)) );
        CHECK_FALSE( sf.get_edges().back()->v2->lat == Approx(41.1 + (i/10.0)) );
        CHECK_FALSE( sf.get_edges().back()->v1->lon == Approx(-83.0 - (i/10.0)) );
        CHECK_FALSE( sf.get_edges().back()->v2->lon == Approx(-84.1 - (i/10.0)) );
    }

    // Do quick read/write file tests.
    shapes::CSVInputFactory input_factory_bad_1("unit-test-data/lib-test-data/test.shapes.bad1");
    input_factory_bad_1.make_shapes();
    shapes::CSVInputFactory input_factory_bad_2("unit-test-data/lib-test-data/test.shapes.bad2");
    CHECK_THROWS_AS(input_factory_bad_2.make_shapes(), std::invalid_argument);
    shapes::CSVInputFactory input_factory_bad_3("unit-test-data/lib-test-data/test.shapes.bad3");
    CHECK_THROWS_AS(input_factory_bad_3.make_shapes(), std::invalid_argument);
    shapes::CSVInputFactory input_factory("unit-test-data/lib-test-data/test.shapes");
    CHECK_NOTHROW(input_factory.make_shapes());
    shapes::CSVOutputFactory output_factory_1("unit-test-data/no-dir/test.shapes.out");
    CHECK_THROWS_AS(output_factory_1.write_shapes(), std::invalid_argument);
    shapes::CSVOutputFactory output_factory("unit-test-data/lib-test-data/test.shapes.out");

    for ( auto& eptr : input_factory.get_edges() ) {
        output_factory.add_edge(eptr);
    }

    for ( auto& gptr : input_factory.get_grids() ) {
        output_factory.add_grid(gptr);
    }

    for ( auto& cptr : input_factory.get_circles() ) {
        output_factory.add_circle(cptr);
    }

    CHECK_NOTHROW(output_factory.write_shapes());
}

TEST_CASE("Entity", "[quad][entity]") {
    SECTION("Conversions") {
        CHECK(geo::to_degrees(0.0) == Approx(0.0));
        CHECK(geo::to_degrees(.5 * geo::kPi) == Approx(90.0));
        CHECK(geo::to_degrees(geo::kPi) == Approx(180.0));
        CHECK(geo::to_degrees(2.0 * geo::kPi) == Approx(360.0));
        CHECK(geo::to_radians(0.0) == Approx(0.0));
        CHECK(geo::to_radians(90.0) == Approx(.5 * geo::kPi));
        CHECK(geo::to_radians(180.0) == Approx(geo::kPi));
        CHECK(geo::to_radians(360.0) == Approx(2.0 * geo::kPi));
    }

    geo::Point pt_a(90.0, 180.0);
    geo::Point pt_b;
    std::stringstream ss;
    ss << pt_a;

    SECTION("Point") {
        CHECK(ss.str() == "90,180");
        CHECK(pt_a == pt_a);
        CHECK(pt_b == pt_b);
    }

    // semi-circumference of Earth (meters)
    const double kSemiCircM = 20037508.3427;
    // Eiffel Tower to Titanic distance (meters)
    const double tToE = 4084152.4248;
    const double tToEApprox = 4167612.32403;
    // Titanic to Eiffel Tower bearing (degrees)
    const double tToEBearing = 60.53401;
    // Eiffel Tower to Titanic bearing (degrees)
    const double eToTBearing = 279.0319;

    geo::Location loc_a(90.0, 180.0);  
    geo::Location loc_b(90.0, 180.0, 1);  
    geo::Location loc_c(-90.0, 180.0);
    geo::Location loc_d(0.0, 0.0);
    geo::Location loc_e(0.00001, 0.0);
    geo::Location loc_f(0.0, 0.00001);
    geo::Location loc_g(0.0001, 0.0);
    geo::Location loc_h(0.0, 0.0001);
    // Eiffel Tower
    geo::Location loc_i(48.857801, 2.295968);
    // Titanic
    geo::Location loc_j(41.728342, -49.948810);
    ss.str("");
    ss << loc_a;

    SECTION("Location") {
        CHECK(ss.str() == "0,90,180");
        // Test the floating point comparisons.
        CHECK(loc_a == loc_a);
        CHECK_FALSE(loc_a == loc_b);
        CHECK_FALSE(loc_d == loc_e); 
        CHECK(loc_d.lat == Approx(loc_e.lat));
        CHECK(loc_d.lon == Approx(loc_f.lon));
        CHECK_FALSE(loc_d.lat == Approx(loc_g.lat));
        CHECK_FALSE(loc_d.lon == Approx(loc_h.lon));
        // Test the distance functions.
        CHECK(geo::Location::distance(loc_a, loc_b) == Approx(0.0));
        CHECK(geo::Location::distance(loc_a, loc_c) == Approx(kSemiCircM));
        CHECK(geo::Location::distance(loc_i, loc_j) == Approx(tToEApprox));
        CHECK(geo::Location::distance(90.0, 180.0, 90.0, 180.0) == Approx(0.0));
        CHECK(geo::Location::distance(90.0, 180.0, -90.0, 180.0) == Approx(kSemiCircM));
        CHECK(geo::Location::distance_haversine(loc_a, loc_a) == Approx(0.0));
        CHECK(geo::Location::distance_haversine(loc_a, loc_c) == Approx(kSemiCircM));
        CHECK(geo::Location::distance_haversine(loc_i, loc_j) == Approx(tToE));
        CHECK(geo::Location::distance(90.0, 180.0, 90.0, 180.0) == Approx(0.0));
        CHECK(geo::Location::distance(90.0, 180.0, -90.0, 180.0) == Approx(kSemiCircM));
        CHECK(loc_a.distance_to(loc_b) == Approx(0.0));
        CHECK(loc_a.distance_to(loc_c) == Approx(kSemiCircM));
        CHECK(loc_i.distance_to(loc_j) == Approx(tToEApprox));
        CHECK(loc_a.distance_to_haversine(loc_a) == Approx(0.0));
        CHECK(loc_a.distance_to_haversine(loc_c) == Approx(kSemiCircM));
        CHECK(loc_i.distance_to_haversine(loc_j) == Approx(tToE));
        // Test the projection functions.
        CHECK(geo::Location::project_position(loc_a, 90.0, kSemiCircM).lat == Approx(-90.0));
        CHECK(geo::Location::project_position(loc_a, 157.0, kSemiCircM / 2.0).lat == Approx(0.0));
        CHECK(geo::Location::project_position(loc_a, 157.0, kSemiCircM / 2.0).lon == Approx(-157.0));
        CHECK(geo::Location::project_position(loc_a, -45.0, kSemiCircM / 2.0).lat == Approx(0.0));
        CHECK(geo::Location::project_position(loc_a, -45.0, kSemiCircM / 2.0).lon == Approx(45.0));
        CHECK(loc_a.project_position(90.0, kSemiCircM).lat == Approx(-90.0));
        CHECK(loc_a.project_position(157.0, kSemiCircM / 2.0).lat == Approx(0.0));
        CHECK(loc_a.project_position(157.0, kSemiCircM / 2.0).lon == Approx(-157.0));
        CHECK(loc_a.project_position(-45.0, kSemiCircM / 2.0).lat == Approx(0.0));
        CHECK(loc_a.project_position(-45.0, kSemiCircM / 2.0).lon == Approx(45.0));
        CHECK(geo::Location::project_position(90.0, 180.0, 90.0, kSemiCircM).lat == Approx(-90.0));
        CHECK(geo::Location::project_position(90.0, 180.0, 157.0, kSemiCircM / 2.0).lat == Approx(0.0));
        CHECK(geo::Location::project_position(90.0, 180.0, 157.0, kSemiCircM / 2.0).lon == Approx(-157.0));
        CHECK(geo::Location::project_position(90.0, 180.0, -45.0, kSemiCircM / 2.0).lat == Approx(0.0));
        CHECK(geo::Location::project_position(90.0, 180.0, -45.0, kSemiCircM / 2.0).lon == Approx(45.0));
        // Test the midpoint functions.
        CHECK(geo::Location::midpoint(loc_a, loc_c).lat == Approx(0.0));
        CHECK(geo::Location::midpoint(loc_a, loc_c).lon == Approx(-180.0));
        CHECK(loc_a.midpoint(loc_c).lat == Approx(0.0));
        CHECK(loc_a.midpoint(loc_c).lon == Approx(-180.0));
        CHECK(geo::Location::midpoint(90.0, 180.0, -90.0, 180.0).lat == Approx(0.0));
        CHECK(geo::Location::midpoint(90.0, 180.0, -90.0, 180.0).lon == Approx(-180.0));
        // Test the bearing functions.
        CHECK(geo::Location::bearing(loc_a, loc_c) == Approx(180.0));
        CHECK(geo::Location::bearing(loc_a, loc_d) == Approx(0.0));
        CHECK(geo::Location::bearing(loc_i, loc_j) == Approx(eToTBearing));
        CHECK(geo::Location::bearing(loc_j, loc_i) == Approx(tToEBearing));
        CHECK(loc_a.bearing_to(loc_c) == Approx(180.0));
        CHECK(loc_a.bearing_to(loc_d) == Approx(0.0));
        CHECK(loc_i.bearing_to(loc_j) == Approx(eToTBearing));
        CHECK(loc_j.bearing_to(loc_i) == Approx(tToEBearing));
        CHECK(geo::Location::bearing(90.0, 180.0, -90.0, 180.0) == Approx(180.0));
        CHECK(geo::Location::bearing(90.0, 180.0, 0.0, 0.0) == Approx(0.0));
    }
    
    // Build a small road network.
    // Pat Head Summit St.
    const double kPHSSDist = 562.537106;
    geo::Vertex::Ptr v_a = std::make_shared<geo::Vertex>(35.952500, -83.932434, 1);
    geo::Vertex::Ptr v_b = std::make_shared<geo::Vertex>(35.948878, -83.928081, 2);
    geo::EdgePtr phss = std::make_shared<geo::Edge>(v_a, v_b, osm::Highway::SECONDARY, 1);
    geo::EdgePtr no_edge = std::make_shared<geo::Edge>(v_a, v_a, osm::Highway::SECONDARY, 1);
    // For coverage.
    geo::EdgePtr phss_cov_1 = std::make_shared<geo::Edge>(v_a, v_b, 1, true);
    geo::EdgePtr phss_cov_2 = std::make_shared<geo::Edge>(*v_a, *v_b, 1, true);
    // Andy Hold West.
    geo::Vertex::Ptr v_c = std::make_shared<geo::Vertex>(35.950715, -83.934971, 3);
    geo::EdgePtr ahw = std::make_shared<geo::Edge>(v_c, v_a, osm::Highway::SECONDARY, 2);
    // Andy Hold East.
    geo::Vertex::Ptr v_d = std::make_shared<geo::Vertex>(35.953302, -83.931344, 4);
    geo::EdgePtr ahe = std::make_shared<geo::Edge>(v_d, v_a, osm::Highway::SECONDARY, 3);
    // 20th St.
    geo::Vertex::Ptr v_e = std::make_shared<geo::Vertex>(35.952175, -83.936688, 5);
    geo::EdgePtr twth = std::make_shared<geo::Edge>(v_e, v_c, osm::Highway::SECONDARY, 4);
    // UT Dr.
    geo::Vertex::Ptr v_f = std::make_shared<geo::Vertex>(35.949813, -83.936214, 6);
    geo::Vertex::Ptr v_g = std::make_shared<geo::Vertex>(35.948272, -83.934421, 7);
    geo::EdgePtr utdr = std::make_shared<geo::Edge>(v_f, v_g, osm::Highway::SECONDARY, 5);
    // Andy Holt West End
    geo::Location ahwe = geo::Location(35.949007, -83.937359, 8);
    // Rec batting cage.
    geo::Location cage = geo::Location(35.951250, -83.931861);
    // midpt on Summit
    geo::Location midsum = geo::Location(35.950689,-83.930257);

    geo::EdgePtrSet edge_set;
    edge_set.insert(phss);
    edge_set.insert(ahw);
    edge_set.insert(ahe);

    SECTION("VertexAndEdges") {
        ss.str("");
        ss << *v_b;
        CHECK(ss.str() == "2,35.948878,-83.92808100000001,0");
        ss.str("");
        ss << *phss;
        CHECK(ss.str() == "1,explicit,secondary,1,35.9525,-83.932434,0,2,35.948878,-83.92808100000001,0");
        CHECK(phss_cov_1->length() == Approx(phss_cov_2->length()));
        CHECK(v_a->add_edges(edge_set));
        CHECK(v_a->degree() == 3);
        CHECK(v_a->outdegree() == 2);
        CHECK(v_e->add_edge(twth));        
        CHECK(v_e->add_edge(ahw));        
        CHECK_FALSE(v_e->add_edge(ahw));        
        CHECK(v_e->degree() == 2);
        CHECK(v_e->outdegree() == 0);
        CHECK(ahw->length_haversine() == Approx(302.9046));
        // update Andy Holt West
        v_c->update_location(ahwe);
        CHECK(ahw->length_haversine() == Approx(590.0603));
        CHECK(v_a->get_incident_edges().size() == 3);
        CHECK(phss->distance_from_point(cage) == Approx(61.3234)); 
        CHECK(phss->distance_from_point(*v_a) == Approx(0.0)); 
        CHECK(phss->distance_from_point(*v_b) == Approx(0.0)); 
        CHECK(no_edge->distance_from_point(*v_b) == Approx(0.0)); 
        // Assume some error here due to the implementation.
        CHECK(phss->distance_from_point(midsum) == Approx(0.03299)); 
        CHECK(phss->get_way_type() == osm::Highway::SECONDARY); 
        CHECK(phss->get_way_type_index() == 3); 
        CHECK(phss->get_way_width() == Approx(17.0)); 
        CHECK(phss->dlatitude() == Approx(-0.00362));
        CHECK(phss->dlongitude() == Approx(0.00435));
        CHECK(phss->length() == Approx(kPHSSDist));
        CHECK(phss->length_haversine() == Approx(kPHSSDist));
        CHECK(phss->bearing() == Approx(135.78563));
        CHECK(phss->intersects(*ahw));
        CHECK_FALSE(no_edge->intersects(*ahw));
        CHECK_FALSE(phss->intersects(*utdr));
        CHECK(phss->intersects(35.952500, -83.932434, 35.950715, -83.934971));
        CHECK_FALSE(phss->intersects(35.949813, -83.936214, 35.948272, -83.934421));
        CHECK(phss->is_explicit());
        CHECK_FALSE(phss->is_implicit());
        CHECK(phss->get_uid() == 1); 
        CHECK(*phss == *phss);
        CHECK_FALSE(*phss == *ahw);
    }

    geo::AreaPtr phss_area = phss->to_area();
    geo::AreaPtr phss_area_long = phss->to_area(10.0);
    geo::AreaPtr phss_area_wide_long = phss->to_area(80.0, 10.0);
    // two very close points, one inside the area the other outside.
    geo::Location inside(35.951128, -83.930657);
    geo::Location outside_1(35.951130, -83.930655);
    geo::Location outside_2(35.952511, -83.932457);
    ss.str("");
    ss << *phss_area;
    
    SECTION("Area") {
        // The check below works on os x but not on Ubunutu.
        // CHECK(ss.str() == "[35.95255324700415,-83.93236639356081, 35.94893124700415,-83.92801339666028, 35.94882475295794,-83.92814860324864, 35.95244675295795,-83.93250160634807, ]");
        // The check below works on Ubuntu but not os x.
        CHECK(ss.str() == "[35.95255324700415,-83.93236639356081, 35.94893124700415,-83.92801339666028, 35.94882475295794,-83.92814860324864, 35.95244675295794,-83.93250160634807, ]");
        CHECK_THROWS(phss->to_area(0.0, 10));
        CHECK_THROWS(phss->to_area(-1.0, 10));
        CHECK_NOTHROW(phss->to_area(10.0, 5));
        CHECK(phss_area->contains(midsum));
        CHECK_FALSE(phss_area->contains(loc_a));
        CHECK(phss_area->contains(inside));
        CHECK_FALSE(phss_area->contains(outside_1));
        CHECK_FALSE(phss_area->outside_edge(-1, inside));
        CHECK_FALSE(phss_area->outside_edge(20, inside));
        CHECK(phss_area->get_type() == "area");
        CHECK(phss_area->outside_edge(0, outside_1));
        CHECK_FALSE(phss_area->outside_edge(1, outside_1));
        CHECK_FALSE(phss_area->outside_edge(2, outside_1));
        CHECK_FALSE(phss_area->outside_edge(3, outside_1));
        CHECK_FALSE(phss_area->contains(outside_2));
        CHECK(phss_area_long->contains(outside_2));
        CHECK(phss_area_wide_long->contains(inside));
        // Check the corners.
        std::vector<geo::Point> corners = phss_area->get_corners();
        geo::Area copy(corners[0], corners[1], corners[2], corners[3]);
        CHECK(corners.size() == 4);
        CHECK(corners[0].lat == Approx(35.952553247));
        CHECK(corners[0].lon == Approx(-83.9323663936));
        CHECK(corners[1].lat == Approx(35.948931247));
        CHECK(corners[1].lon == Approx(-83.9280133967));
        CHECK(corners[2].lat == Approx(35.948824753));
        CHECK(corners[2].lon == Approx(-83.9281486032));
        CHECK(corners[3].lat == Approx(35.952446753));
        CHECK(corners[3].lon == Approx(-83.9325016063));
        CHECK(geo::Location::distance_haversine(corners[0].lat, corners[0].lon, corners[1].lat, corners[1].lon) == Approx(kPHSSDist));
        CHECK(geo::Location::distance_haversine(corners[2].lat, corners[2].lon, corners[3].lat, corners[3].lon) == Approx(kPHSSDist));
        CHECK(geo::Location::distance_haversine(corners[0].lat, corners[0].lon, corners[3].lat, corners[3].lon) == Approx(17.0));
        CHECK(geo::Location::distance_haversine(corners[1].lat, corners[1].lon, corners[2].lat, corners[2].lon) == Approx(17.0));
        CHECK(phss_area->get_poly_string() == "-83.93236639,35.95255325,0 -83.9280134,35.94893125,0 -83.9281486,35.94882475,0 -83.93250161,35.95244675,0 -83.93236639,35.95255325,0");
    }

    geo::Circle c1(cage, 10.0);
    // contains itself
    geo::Circle c2(cage, 0.0);
    // contains nothing (cage)
    geo::Circle c3(35.951250, -83.931861, -1.0);
    geo::Location c_inside(35.951295, -83.931768);
    geo::Location c_outside(35.951297, -83.931765);
    ss.str("");
    ss << c1;

    SECTION("Circle") {
        CHECK(ss.str() == "35.95125, -83.931861, 10");
        CHECK_FALSE(c1.contains(loc_a));
        CHECK(c1.contains(c_inside));
        CHECK_FALSE(c1.contains(c_outside));
        CHECK_FALSE(c2.contains(c_outside));    
        CHECK_FALSE(c2.contains(c_inside));    
        CHECK(c2.contains(c1));    
        CHECK(c1.contains(c2));    
        CHECK_FALSE(c3.contains(c2));    
    }

    geo::Location sw1(35.951853, -83.932832);
    geo::Location ne1(35.953642, -83.929975);
    geo::Bounds b1(sw1, ne1);
    geo::Location sw2(35.952062, -83.931951);
    geo::Location ne2(35.952320, -83.931718);
    geo::Bounds b2(sw2, ne2);
    geo::Location sw3(35.951644, -83.931921);
    geo::Location ne3(35.951953, -83.930746);
    geo::Bounds b3(sw3, ne3);
    geo::Location sw4(35.950260, -83.931860);
    geo::Location ne4(35.950601, -83.931282);
    geo::Bounds b4(sw4, ne4);
    geo::Location b_inside(35.952670, -83.931534);
    geo::Circle c4(b_inside, 10.0);
    geo::Circle c5(b_inside, 120.0);
    geo::Circle c6(b_inside, 1200.0);
    ss.str("");
    ss << b1; 

    SECTION("Bounds") {
        CHECK(ss.str() == "35.951853,-83.932832,35.953642,-83.929975");
        CHECK_FALSE(b1.contains(loc_a));
        CHECK_FALSE(c1 == c2);
        CHECK(b1.contains(sw1));
        CHECK(b1.contains(ne1));
        CHECK(b1.contains(b_inside));
        CHECK(b1.intersects(b_inside, loc_a));
        CHECK_FALSE(b1.contains(*phss));
        CHECK(phss_area->touches(b1));
        CHECK(phss_area->touches(b2));
        CHECK(phss_area->touches(b3));
        CHECK(b1.intersects(*phss));
        CHECK(b1.contains(*ahe));
        CHECK_FALSE(b1.intersects(*ahe));
        CHECK(b1.contains_or_intersects(*phss));
        CHECK_FALSE(b1.contains(*utdr));    
        CHECK(b1.contains(c4));
        CHECK(b1.intersects(c5));
        CHECK_FALSE(b1.intersects(c6));
        CHECK_FALSE(b1.contains_or_intersects(c6));
        CHECK(b1.west_midpoint().lat == Approx(35.95274));
        CHECK(b1.west_midpoint().lon == Approx(-83.9328));
        CHECK(b1.east_midpoint().lat == Approx(35.95274));
        CHECK(b1.east_midpoint().lon == Approx(-83.9299));
        CHECK(b1.north_midpoint().lat == Approx(35.9536));
        CHECK(b1.north_midpoint().lon == Approx(-83.93140));
        CHECK(b1.south_midpoint().lat == Approx(35.95185));
        CHECK(b1.south_midpoint().lon == Approx(-83.931403));
        CHECK(b1.center().lat == Approx(b1.east_midpoint().lat));
        CHECK(b1.center().lat == Approx(b1.west_midpoint().lat));
        CHECK(b1.center().lon == Approx(b1.south_midpoint().lon));
        CHECK(b1.center().lon == Approx(b1.north_midpoint().lon));
        CHECK(b1.width() == Approx(0.002857));
        CHECK(b1.height() == Approx(0.001789));
    }

    geo::Location nw(35.953642, -83.932832);
    geo::Grid g1(b1, 0, 0);
//    geo::Grid::GridPtrVector grids = geo::Grid::build_grid(nw, .7, 35.951853, -83.929975);
//    
//    SECTION("Grid") {
//        CHECK(grids.size() == 1);
//    }

    geo::Grid::GridPtrVector grids = geo::Grid::build_grid(nw, 10, 35.951853, -83.929975);
    ss.str("");
    ss << g1;

    SECTION("Grid") {
        CHECK(ss.str() == "35.951853,-83.932832,35.953642,-83.929975,0,0");
        CHECK(grids.size() == 520);

        // Each grid should be 10 by 10 meters.
        for (int i = 0; i < 520; ++i) {
            CHECK(geo::Location::distance_haversine(grids[i]->nw.lat, grids[i]->nw.lon, grids[i]->sw.lat, grids[i]->sw.lon) == Approx(10.0));
            CHECK(geo::Location::distance_haversine(grids[i]->nw.lat, grids[i]->nw.lon, grids[i]->ne.lat, grids[i]->ne.lon) == Approx(10.0));
            CHECK(geo::Location::distance_haversine(grids[i]->se.lat, grids[i]->se.lon, grids[i]->sw.lat, grids[i]->sw.lon) == Approx(10.0));
            CHECK(geo::Location::distance_haversine(grids[i]->se.lat, grids[i]->se.lon, grids[i]->ne.lat, grids[i]->ne.lon) == Approx(10.0));

            // Check the circle is only on one grid -> the grids are disjoint.
            if (i == 271) {
                CHECK(grids[i]->contains(b_inside));
            } else {
                CHECK_FALSE(grids[i]->contains(b_inside));
            }

            // All grids should touch the bounds.
            CHECK(grids[i]->touches(b1));
        }

        CHECK(g1.touches(b2));
        CHECK_FALSE(g1.touches(b4));
    }
    
    geo::Grid::CPtr g2_ptr = grids[271];
    geo::Location touch_test(35.952649, -83.933059);
    geo::Circle c_touch_test(touch_test, 25.0);

    SECTION("Entity") {
        // basic entity test
        CHECK(b_inside.get_type() == "location"); 
        CHECK(phss->get_type() == "edge");
        CHECK(c1.get_type() == "circle");
        CHECK(g2_ptr->get_type() == "grid");
        CHECK(b_inside.touches(b1));
        CHECK(phss->touches(b1));
        CHECK(g2_ptr->touches(b1));
        CHECK(c4.touches(b1));
        CHECK(c5.touches(b1));
        CHECK(c6.touches(b1));
        CHECK(c_touch_test.touches(b1));
    }
}

TEST_CASE("Quad Tree", "[quad]") {
    // borrowing network from entity tests
    geo::Vertex::Ptr v_a = std::make_shared<geo::Vertex>(35.952500, -83.932434, 1);
    geo::Vertex::Ptr v_b = std::make_shared<geo::Vertex>(35.948878, -83.928081, 2);
    geo::EdgePtr phss = std::make_shared<geo::Edge>(v_a, v_b, osm::Highway::SECONDARY, 1);
    // Andy Hold West.
    geo::Vertex::Ptr v_c = std::make_shared<geo::Vertex>(35.950715, -83.934971, 3);
    geo::EdgePtr ahw = std::make_shared<geo::Edge>(v_c, v_a, osm::Highway::SECONDARY, 2);
    // Andy Hold East.
    geo::Vertex::Ptr v_d = std::make_shared<geo::Vertex>(35.953302, -83.931344, 4);
    geo::EdgePtr ahe = std::make_shared<geo::Edge>(v_d, v_a, osm::Highway::SECONDARY, 3);
    // 20th St.
    geo::Vertex::Ptr v_e = std::make_shared<geo::Vertex>(35.952175, -83.936688, 5);
    geo::EdgePtr twth = std::make_shared<geo::Edge>(v_e, v_c, osm::Highway::SECONDARY, 4);
    // UT Dr.
    geo::Vertex::Ptr v_f = std::make_shared<geo::Vertex>(35.949813, -83.936214, 6);
    geo::Vertex::Ptr v_g = std::make_shared<geo::Vertex>(35.948272, -83.934421, 7);
    geo::EdgePtr utdr = std::make_shared<geo::Edge>(v_f, v_g, osm::Highway::SECONDARY, 5);
    geo::Point test_point_1(35.951959, -83.931815);
    geo::Point test_point_2(35.949098, -83.935403);
    geo::Point test_point_3(90.0, 180.0);
    // horizontal split
    geo::Point sw(35.948378, -83.936072);
    geo::Point ne(35.953811, -83.928997);
    Quad::Ptr quad_ptr = std::make_shared<Quad>(sw, ne);
    std::stringstream ss;
    ss << *quad_ptr;
    
    SECTION("Basic") {
        CHECK(ss.str() == "Quad: {35.948378,-83.936072, 35.953811,-83.928997} element count: 0 level: 0 children: 0 fuzzy: {35.9478347,-83.9367795, 35.95435430000001,-83.92828949999999, 0.006519600000004289, 0.00849000000000899}");
        // inserts
        CHECK(Quad::insert(quad_ptr, phss));     
        CHECK(Quad::insert(quad_ptr, ahw));     
        CHECK(Quad::insert(quad_ptr, ahe));     
        CHECK(Quad::insert(quad_ptr, twth));     
        CHECK(Quad::insert(quad_ptr, utdr));
        // retrievals 
        // All retrievals should give 5 elements since the splitting criteria 
        // will not be hit untill 32 elements are inserted.
        geo::Entity::PtrList element_list = quad_ptr->retrieve_elements(test_point_1);
        CHECK(element_list.size() == 5);
        element_list = quad_ptr->retrieve_elements(*v_a);
        CHECK(element_list.size() == 5);
        element_list = quad_ptr->retrieve_elements(*v_c);
        CHECK(element_list.size() == 5);
        // Try to retrieve either end of UT drive.
        // This will fail since they are outside the quad bounds.
        element_list = quad_ptr->retrieve_elements(*v_f);
        CHECK(element_list.size() == 0);
        element_list = quad_ptr->retrieve_elements(*v_g);
        CHECK(element_list.size() == 0);
        // Try to retrieve the edge using a point on the road.
        element_list = quad_ptr->retrieve_elements(test_point_2);
        CHECK(element_list.size() == 5);
        // Try to retrieve something outsdie the quad.
        CHECK_FALSE(quad_ptr->retrieve_bounds(test_point_3));
    }    

    SECTION("Structural") {
        // re-insert
        Quad::insert(quad_ptr, phss);     
        Quad::insert(quad_ptr, ahw);     
        Quad::insert(quad_ptr, ahe);     
        Quad::insert(quad_ptr, twth);     
        Quad::insert(quad_ptr, utdr);
        std::vector<geo::Bounds::Ptr> bounds_all = Quad::retrieve_all_bounds(quad_ptr);
        CHECK(bounds_all.size() == 1);
        bounds_all = Quad::retrieve_all_bounds(quad_ptr, false, true);
        CHECK(bounds_all.size() == 1);
        bounds_all = Quad::retrieve_all_bounds(quad_ptr, true, false);
        CHECK(bounds_all.size() == 1);
        bounds_all = Quad::retrieve_all_bounds(quad_ptr, true, true);
        CHECK(bounds_all.size() == 1);

        // test the bounds of some retreivals
        geo::Bounds::Ptr b_ret = quad_ptr->retrieve_bounds(test_point_1, true);
        CHECK(b_ret->sw.lat == Approx(35.9478));
        CHECK(b_ret->sw.lon == Approx(-83.9367));
        CHECK(b_ret->ne.lat == Approx(35.95435));
        CHECK(b_ret->ne.lon == Approx(-83.9282));
    }

    // 4-way split
    geo::Point sw_2(35.948378, -83.936072);
    geo::Point ne_2(35.955110, -83.928997);
    Quad::Ptr quad_ptr_2 = std::make_shared<Quad>(sw_2, ne_2);

    // vertical split
    geo::Point sw_3(35.948378, -83.934448);
    geo::Point ne_3(35.955110, -83.928997);
    Quad::Ptr quad_ptr_3 = std::make_shared<Quad>(sw_3, ne_3);

    SECTION("Splitting") {
        geo::Location::Ptr loc_ptr = std::make_shared<geo::Location>(35.951959, -83.931815, 33);
    
        // Fill up a single leaf. Horizontal split.
        for (int i = 0; i < Quad::MAX_ELEMENTS; ++i) {
            geo::Location::Ptr test_loc_ptr = std::make_shared<geo::Location>(35.951959, -83.931815, i);
            Quad::insert(quad_ptr, test_loc_ptr);
        }
        // Fill up a single leaf. 4-way split.
        for (int i = 0; i < Quad::MAX_ELEMENTS + 1; ++i) {
            geo::Location::Ptr test_loc_ptr = std::make_shared<geo::Location>(35.951959, -83.931815, i);
            Quad::insert(quad_ptr_2, test_loc_ptr);
        }
        // Fill up a single leaf. Vertical split.
        for (int i = 0; i < Quad::MAX_ELEMENTS + 1; ++i) {
            geo::Location::Ptr test_loc_ptr = std::make_shared<geo::Location>(35.951959, -83.931815, i);
            Quad::insert(quad_ptr_3, test_loc_ptr);
        }
        geo::Bounds::Ptr b_ret = quad_ptr->retrieve_bounds(*loc_ptr);
        CHECK(geo::Location::distance_haversine(b_ret->nw.lat, b_ret->nw.lon, b_ret->sw.lat, b_ret->sw.lon) == Approx(604.7987));
        // Force the quad to split down to the minimum degree,
        // by entering the same point 33 times.
        // This splits the test quad vertically.
        Quad::insert(quad_ptr, loc_ptr);
        b_ret = quad_ptr->retrieve_bounds(*loc_ptr);
        // Test for vertical split.
        CHECK(geo::Location::distance_haversine(b_ret->nw.lat, b_ret->nw.lon, b_ret->ne.lat, b_ret->ne.lon) == Approx(318.771477));
        Quad::insert(quad_ptr, loc_ptr);
        b_ret = quad_ptr->retrieve_bounds(*loc_ptr);
        // Check degrees are good.
        CHECK(b_ret->ne.lon - b_ret->nw.lon >= Approx(0.003));
        CHECK(b_ret->ne.lat - b_ret->sw.lat >= Approx(0.003));
        // Check element size.
        geo::Entity::PtrList element_list = quad_ptr->retrieve_elements(*loc_ptr);
        CHECK(element_list.size() == 34);
        CHECK(Quad::retrieve_all_bounds(quad_ptr).size() == 3);
        CHECK(Quad::retrieve_all_bounds(quad_ptr, false, true).size() == 3);
        CHECK(Quad::retrieve_all_bounds(quad_ptr, true, true).size() == 2);
    }

}

TEST_CASE("DI Algorithm", "[map match][intersection count][critical interval][privacy interval][de-identification]") {
    Quad::Ptr qptr = buildTestQuadTree();

    // map matching, intersection counting, critical intervals
    SECTION("Feature Detection") {
        BSMP1::BSMP1CSVTrajectoryFactory factory;
        trajectory::Trajectory traj = factory.make_trajectory("unit-test-data/lib-test-data/utk_test.csv");
        MapFitter mf(qptr, 1.0, .5);
        mf.fit(traj);
        ImplicitMapFitter imf{36, 10};
        imf.fit(traj);

        for (uint64_t i = 0; i < traj.size(); ++i) {
            if (i < 34 || (i > 57 && i < 96) || i > 145) {
                CHECK(traj[i]->is_explicitly_fit());
            } else {
                CHECK((traj[i]->is_implicitly_fit()));
            }
        }

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        StartEndIntervals sei;
        trajectory::Interval::PtrList se_intervals = sei.get_start_end_intervals(traj);

        CHECK(se_intervals.size() == 2);
        CHECK(se_intervals[0]->left() == 0);
        CHECK(se_intervals[0]->right() == 1);
        CHECK(se_intervals[0]->get_aux_str() == "start_pt");
        CHECK(se_intervals[1]->left() == traj.size() - 1);
        CHECK(se_intervals[1]->right() == traj.size());
        CHECK(se_intervals[1]->get_aux_str() == "end_pt");

        // Set the speed becuase of how the data was generated.
        Detector::TurnAround tad{20, 30.0, 100.0, 90.0};
        trajectory::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(traj);

        CHECK(ta_critical_intervals.size() == 2);
        CHECK(ta_critical_intervals[0]->left() == 33);
        CHECK(ta_critical_intervals[0]->right() == 58);
        CHECK(ta_critical_intervals[0]->get_aux_str() == "ta_fit");
        CHECK(ta_critical_intervals[1]->left() == 107);
        CHECK(ta_critical_intervals[1]->right() == 130);
        CHECK(ta_critical_intervals[1]->get_aux_str() == "ta");

        // Make the time smaller to handle the test data.
        Detector::Stop stop_detector{1.0, 50.0, 2.5};
        trajectory::Interval::PtrList stop_critical_intervals = stop_detector.find_stops(traj);

        CHECK(stop_critical_intervals.size() == 2);
        CHECK(stop_critical_intervals[0]->left() == 0);
        CHECK(stop_critical_intervals[0]->right() == 10);
        CHECK(stop_critical_intervals[0]->get_aux_str() == "stop");
        CHECK(stop_critical_intervals[1]->left() == 177);
        CHECK(stop_critical_intervals[1]->right() == 187);
        CHECK(stop_critical_intervals[1]->get_aux_str() == "stop");

        IntervalMarker im( { ta_critical_intervals, stop_critical_intervals, se_intervals } );
        im.mark_trajectory( traj );

        for (auto& tp : traj) {
            uint64_t i = tp->get_index();

            if (tp->is_critical()) {
                CHECK((i < 10 || (i > 32 && i < 58) || (i > 106 && i < 130) || ( i > 176 && i < 187) || i == 280));
            }
        }
    }

    SECTION("Out Degree Max") {
        BSMP1::BSMP1CSVTrajectoryFactory factory;
        trajectory::Trajectory traj = factory.make_trajectory("unit-test-data/lib-test-data/utk_test.csv");
        MapFitter mf(qptr, 1.0, .5);
        mf.fit(traj);
        ImplicitMapFitter imf{36, 10};
        imf.fit(traj);

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        StartEndIntervals sei;
        trajectory::Interval::PtrList se_intervals = sei.get_start_end_intervals(traj);

        // Set the speed becuase of how the data was generated.
        Detector::TurnAround tad{20, 30.0, 100.0, 90.0};
        trajectory::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(traj);

        IntervalMarker im( { ta_critical_intervals, se_intervals } );
        im.mark_trajectory( traj );
        
        // First check max out-degree coverage for the two intersections.
        PrivacyIntervalFinder pif(10000.0, 
                                  10000.0, 
                                  1, 
                                  11000.0, 
                                  11000.0, 
                                  4, 
                                  0.0,
                                  0.0, 
                                  0.0);

        trajectory::Interval::PtrList priv_intervals = pif.find_intervals(traj);

        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 33);
        CHECK(priv_intervals[0]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 107);
        CHECK(priv_intervals[1]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[2]->left() == 130);
        CHECK(priv_intervals[2]->right() == 277);
        CHECK(priv_intervals[2]->get_aux_str() == "forward:max_out_degree");
        CHECK(priv_intervals[3]->left() == 277);
        CHECK(priv_intervals[3]->right() == 280);
        CHECK(priv_intervals[3]->get_aux_str() == "backward:pi");
    }

    SECTION("Direct Distance Max") {
        BSMP1::BSMP1CSVTrajectoryFactory factory;
        trajectory::Trajectory traj = factory.make_trajectory("unit-test-data/lib-test-data/utk_test.csv");
        MapFitter mf(qptr, 1.0, .5);
        mf.fit(traj);
        ImplicitMapFitter imf{36, 10};
        imf.fit(traj);

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        StartEndIntervals sei;
        trajectory::Interval::PtrList se_intervals = sei.get_start_end_intervals(traj);

        // Set the speed becuase of how the data was generated.
        Detector::TurnAround tad{20, 30.0, 100.0, 90.0};
        trajectory::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(traj);

        IntervalMarker im( { ta_critical_intervals, se_intervals } );
        im.mark_trajectory( traj );
    
        // Check max direct distance.
        PrivacyIntervalFinder pif(280.0, 
                                  10000.0, 
                                  10, 
                                  300.0, 
                                  11000.0, 
                                  11, 
                                  0.0,
                                  0.0, 
                                  0.0);

        trajectory::Interval::PtrList priv_intervals = pif.find_intervals(traj);

        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 33);
        CHECK(priv_intervals[0]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 107);
        CHECK(priv_intervals[1]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[2]->left() == 130);
        CHECK(priv_intervals[2]->right() == 241);
        CHECK(priv_intervals[2]->get_aux_str() == "forward:max_dist");
        CHECK(priv_intervals[3]->left() == 241);
        CHECK(priv_intervals[3]->right() == 280);
        CHECK(priv_intervals[3]->get_aux_str() == "backward:pi");
    }

    SECTION("Manhattan Distance Max") {
        BSMP1::BSMP1CSVTrajectoryFactory factory;
        trajectory::Trajectory traj = factory.make_trajectory("unit-test-data/lib-test-data/utk_test.csv");
        MapFitter mf(qptr, 1.0, .5);
        mf.fit(traj);
        ImplicitMapFitter imf{36, 10};
        imf.fit(traj);

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        StartEndIntervals sei;
        trajectory::Interval::PtrList se_intervals = sei.get_start_end_intervals(traj);

        // Set the speed becuase of how the data was generated.
        Detector::TurnAround tad{20, 30.0, 100.0, 90.0};
        trajectory::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(traj);

        IntervalMarker im( { ta_critical_intervals, se_intervals } );
        im.mark_trajectory( traj );
        
        // First check max out-degree coverage for the two intersections.
        PrivacyIntervalFinder pif(10000.0, 
                                  280.0, 
                                  10, 
                                  11000.0, 
                                  330.0, 
                                  11, 
                                  0.0,
                                  0.0, 
                                  0.0);

        trajectory::Interval::PtrList priv_intervals = pif.find_intervals(traj);

        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 33);
        CHECK(priv_intervals[0]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 107);
        CHECK(priv_intervals[1]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[2]->left() == 130);
        CHECK(priv_intervals[2]->right() == 216);
        CHECK(priv_intervals[2]->get_aux_str() == "forward:max_dist");
        CHECK(priv_intervals[3]->left() == 216);
        CHECK(priv_intervals[3]->right() == 280);
        CHECK(priv_intervals[3]->get_aux_str() == "backward:pi");
    }

    SECTION("Min Metrics") {
        BSMP1::BSMP1CSVTrajectoryFactory factory;
        trajectory::Trajectory traj = factory.make_trajectory("unit-test-data/lib-test-data/utk_test.csv");
        MapFitter mf(qptr, 1.0, .5);
        mf.fit(traj);
        ImplicitMapFitter imf{36, 10};
        imf.fit(traj);

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        StartEndIntervals sei;
        trajectory::Interval::PtrList se_intervals = sei.get_start_end_intervals(traj);

        // Set the speed becuase of how the data was generated.
        Detector::TurnAround tad{20, 30.0, 100.0, 90.0};
        trajectory::Interval::PtrList ta_critical_intervals = tad.find_turn_arounds(traj);

        IntervalMarker im( { ta_critical_intervals, se_intervals } );
        im.mark_trajectory( traj );
        
        // First check max out-degree coverage for the two intersections.
        PrivacyIntervalFinder pif(300.0, 
                                  300.0, 
                                  2, 
                                  11000.0, 
                                  11000.0, 
                                  10, 
                                  0.0,
                                  0.0, 
                                  0.0);

        trajectory::Interval::PtrList priv_intervals = pif.find_intervals(traj);

        CHECK(priv_intervals[0]->left() == 1);
        CHECK(priv_intervals[0]->right() == 33);
        CHECK(priv_intervals[0]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[1]->left() == 58);
        CHECK(priv_intervals[1]->right() == 107);
        CHECK(priv_intervals[1]->get_aux_str() == "forward:ci");
        CHECK(priv_intervals[2]->left() == 130);
        CHECK(priv_intervals[2]->right() == 277);
        CHECK(priv_intervals[2]->get_aux_str() == "forward:min");
        CHECK(priv_intervals[3]->left() == 277);
        CHECK(priv_intervals[3]->right() == 280);
        CHECK(priv_intervals[3]->get_aux_str() == "backward:pi");
    }

    SECTION("DI") {
        BSMP1::BSMP1CSVTrajectoryFactory factory;
        trajectory::Trajectory traj = factory.make_trajectory("unit-test-data/lib-test-data/utk_test.csv");
        MapFitter mf(qptr, 1.0, .5);
        mf.fit(traj);
        ImplicitMapFitter imf{36, 10};
        imf.fit(traj);

        IntersectionCounter ic{};
        ic.count_intersections(traj);

        StartEndIntervals sei;
        trajectory::Interval::PtrList se_intervals = sei.get_start_end_intervals(traj);

        IntervalMarker im( { se_intervals } );
        im.mark_trajectory( traj );
        
        // First check max out-degree coverage for the two intersections.
        PrivacyIntervalFinder pif(10.0, 
                                  10.0, 
                                  0, 
                                  11000.0, 
                                  11000.0, 
                                  10, 
                                  0.0,
                                  0.0, 
                                  0.0);

        trajectory::Interval::PtrList priv_intervals = pif.find_intervals(traj);

        PrivacyIntervalMarker pim({ priv_intervals });
        pim.mark_trajectory(traj);
        
        for (auto& tp : traj) {
            if (tp->is_private()) {
                uint64_t index = tp->get_index();

                CHECK(((index > 0 && index < 34) || (index > 167 && index < 280)));
            }
        }

        DeIdentifier di;

        trajectory::Trajectory di_traj = di.de_identify(traj);

        CHECK(di_traj.size() == 134);
    }

    SECTION("Error And Point Counter") {
        BSMP1::BSMP1CSVTrajectoryFactory factory_1;
        instrument::PointCounter point_counter_1; 
        trajectory::Trajectory traj_1 = factory_1.make_trajectory("unit-test-data/lib-test-data/utk_test.csv", point_counter_1);

        MapFitter mf_1(qptr, 1.0, .5);
        mf_1.fit(traj_1);
        ImplicitMapFitter imf_1{36, 10};
        imf_1.fit(traj_1);

        IntersectionCounter ic_1{};
        ic_1.count_intersections(traj_1);

        StartEndIntervals sei_1;
        trajectory::Interval::PtrList se_intervals_1 = sei_1.get_start_end_intervals(traj_1);

        Detector::Stop stop_detector{1.0, 50.0, 2.5};
        trajectory::Interval::PtrList stop_critical_intervals = stop_detector.find_stops(traj_1);

        IntervalMarker im_1( { se_intervals_1, stop_critical_intervals } );
        im_1.mark_trajectory( traj_1 );
        
        // First check max out-degree coverage for the two intersections.
        PrivacyIntervalFinder pif_1(10.0, 
                                  10.0, 
                                  0, 
                                  11000.0, 
                                  11000.0, 
                                  10, 
                                  0.0,
                                  0.0, 
                                  0.0);

        trajectory::Interval::PtrList priv_intervals_1 = pif_1.find_intervals(traj_1);

        PrivacyIntervalMarker pim_1({ priv_intervals_1 });
        pim_1.mark_trajectory(traj_1);

        DeIdentifier di;
        di.de_identify(traj_1, point_counter_1);

        CHECK(point_counter_1.n_points == 281);
        CHECK(point_counter_1.n_invalid_field_points == 0);
        CHECK(point_counter_1.n_invalid_geo_points == 0);
        CHECK(point_counter_1.n_invalid_heading_points == 0);
        CHECK(point_counter_1.n_error_points == 0);
        CHECK(point_counter_1.n_ci_points == 21);
        CHECK(point_counter_1.n_pi_points == 126);

        BSMP1::BSMP1CSVTrajectoryFactory factory_2;
        instrument::PointCounter point_counter_2; 
        trajectory::Trajectory traj_2 = factory_2.make_trajectory("unit-test-data/lib-test-data/utk_err_test.csv", point_counter_2);

        ErrorCorrector ec(50);
        ec.correct_error(traj_2, factory_2.get_uid(), point_counter_2);

        MapFitter mf_2(qptr, 1.0, .5);
        mf_2.fit(traj_2);
        ImplicitMapFitter imf_2{36, 10};
        imf_2.fit(traj_2);

        IntersectionCounter ic_2{};
        ic_2.count_intersections(traj_2);

        StartEndIntervals sei_2;
        trajectory::Interval::PtrList se_intervals_2 = sei_2.get_start_end_intervals(traj_2);

        IntervalMarker im_2( { se_intervals_2 } );
        im_2.mark_trajectory( traj_2 );

        PrivacyIntervalFinder pif_2(10.0, 
                                  10.0, 
                                  0, 
                                  11000.0, 
                                  11000.0, 
                                  10, 
                                  0.0,
                                  0.0, 
                                  0.0);
        
        trajectory::Interval::PtrList priv_intervals_2 = pif_2.find_intervals(traj_2);

        PrivacyIntervalMarker pim_2({ priv_intervals_2 });
        pim_2.mark_trajectory(traj_2);
        
        di.de_identify(traj_2, point_counter_2);

        CHECK(point_counter_2.n_points == 138);
        CHECK(point_counter_2.n_invalid_field_points == 0);
        CHECK(point_counter_2.n_invalid_geo_points == 3);
        CHECK(point_counter_2.n_invalid_heading_points == 1);
        CHECK(point_counter_2.n_error_points == 4);
        CHECK(point_counter_2.n_ci_points == 2);
        CHECK(point_counter_2.n_pi_points == 128);

        instrument::PointCounter point_counter_3 = point_counter_1 + point_counter_2;

        CHECK(point_counter_3.n_points == 419);
        CHECK(point_counter_3.n_invalid_field_points == 0);
        CHECK(point_counter_3.n_invalid_geo_points == 3);
        CHECK(point_counter_3.n_invalid_heading_points == 1);
        CHECK(point_counter_3.n_error_points == 4);
        CHECK(point_counter_3.n_ci_points == 23);
        CHECK(point_counter_3.n_pi_points == 254);
    }
}
