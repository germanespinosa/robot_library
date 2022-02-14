/* creates paths for robot that are navigatable
 * considers the robot size and avoids gaps adjacent to occlusions
 */

#include <cell_world/paths.h>
#include <cell_world/map.h>
#include <cell_world.h>
#include <utility>
#include <numeric>
#include <iostream>
#include <vector>
using namespace std;
using namespace cell_world;

vector<int> get_pattern(int start){
    /* given occluded cell in hex circle return the cells that need to be check for occlusdions*/
    int pattern_array[] = {2,3,4,5,0,1,2,3};
    vector<int> concat_pattern;
    for (int i = start; i<=(start+2); i++){
        concat_pattern.push_back(pattern_array[i]); // append
    }
    return concat_pattern;
}

bool adj_occlusions(auto &coord, Map &map2, Map &map1){
    int * pattern_arrayp;
    int cell_num = 0;
    Coordinates cell;
    // create coordinate array
    Coordinates ne = Coordinates(coord.x + 1, coord.y + 1);
    Coordinates e = Coordinates(coord.x + 2, coord.y);
    Coordinates se = Coordinates(coord.x + 1, coord.y - 1);
    Coordinates sw = Coordinates(coord.x - 1, coord.y - 1);
    Coordinates w = Coordinates(coord.x - 2, coord.y);
    Coordinates nw = Coordinates(coord.x - 1, coord.y + 1);
    Coordinates new_coordinates[] = {ne, e, se, sw, w, nw};

    // loop through all surrounding cells
    for (auto &c: new_coordinates){
        // check if new coordinate is in world
        if (map1.find(c) != Not_found) {
            // check if new coordinate is occluded
            if (map1[c].occluded) {
                // get pattern to check
                auto check_cells= get_pattern(cell_num);
                // check nonadjacent cells of the found occluded cell
                for (int i:check_cells){
                    cell = new_coordinates[i];
                    // check if cell exists in world
                    if(map1.find(cell) != Not_found){
                        // if cell exits in world check if it is occluded
                        if (map1[cell].occluded) return true;
                    }
                }
                //return false; // return false if no conflicts detected
            }
        }
        cell_num++;
    }
    return false;
}



//bool  adjacent_occlusions(auto &coord, Map &map2, Map &map1){
//    /* If a cell is open check all surrounding occlusions
//     * If an occlusion pattern that is  not navigable is detected make is "fake occluded"
//     */
//
//    //cout << "COORD ..........................." << coord << endl;
//    int prev_x;
//    int prev_y;
//    float dist_sq;
//    int occlusion_count = 0;
//    int prev_loop_count;
//    int loop_count =0;
//    //map2[coord].id;
//
//    // create coordinate array
//    Coordinates ne = Coordinates(coord.x + 1, coord.y + 1);
//    Coordinates e = Coordinates(coord.x + 2, coord.y);
//    Coordinates se = Coordinates(coord.x + 1, coord.y - 1);
//    Coordinates sw = Coordinates(coord.x - 1, coord.y - 1);
//    Coordinates w = Coordinates(coord.x - 2, coord.y);
//    Coordinates nw = Coordinates(coord.x - 1, coord.y + 1);
//    Coordinates new_coordinates[] = {ne, e, se, sw, w, nw};
//    Coordinates prev_occlusions[] = {};
//
//
//    // loop through all surrounding cells
//    for (auto &c: new_coordinates){
//
//
//        // check if new coordinate is in world
//        if (map1.find(c) != Not_found) {
//
//            // check if new coordinate is occluded
//            if (map1[c].occluded) {
//
//
//
//                // compute distance between current occluded cell
//                if (occlusion_count > 0) {
//                    //cout << occlusion_count << endl;
//
//                    dist_sq = (pow((c.x - prev_x), 2) + pow((c.y - prev_y), 2));
//
//                    if (dist_sq == 8 || dist_sq == 16 || dist_sq == 10) {
//                        cout << coord << endl;
//                        cout << dist_sq << endl;
//                        return true;
//                    }
//
//                    // if distance is 2 and the cells are not adjacent to eachother
////                else if((dist==2) && (loop_count  - prev_loop_count > 1) ) {
////                    cout << "else if ..........." << coord << endl;
////                    return true;
////                }
//                    occlusion_count++;
//                }
//
//
//            prev_loop_count = loop_count;
//            loop_count++;
//            }
//        }
//    return false;
//    }

int main(){

    // load world and create new_world to modify
    string occlusions_name = "10_05";
    auto wc = Resources::from("world_configuration").key("hexagonal").get_resource<World_configuration>();
    auto wi = Resources::from("world_implementation").key("hexagonal").key("mice").get_resource<World_implementation>();
    auto occlusions = Resources::from("cell_group").key("hexagonal").key(occlusions_name).key("occlusions").get_resource<Cell_group_builder>();
    World world1(wc, wi, occlusions);
    World world2 = world1;

    // modify world by occluding cells
    Cell_group cg1 = world1.create_cell_group();
    Cell_group cg2 = world2.create_cell_group();
    Map map1 (cg1);
    Map map2 (cg2);
    map2[Coordinates(0,0)].occluded;


    //cout << world_new.cells[0].coordinates << endl;
    auto free_cells = map2.cells.free_cells();
    cout << free_cells << endl;
    //cout << world2 << endl;
    cout << map2.cells.occluded_cells();
    //cout << cg2[map2.find(Coordinates(-11,1))] << endl;

    //cout << cg[map.find(world.cells[0].coordinates)] << endl;
    int added_occ_count = 0;

    for(auto &cell : free_cells){
        //bool occluded_bool = adjacent_occlusions(cell.get().coordinates, map2, map1);
        //cout << "cells ....." << cell << endl;
        bool occluded_bool = adj_occlusions(cell.get().coordinates, map2, map1);
        int index = map2.find(cell.get().coordinates);

//        cout << ' ' << index << endl;
//        cout << ' ' << cell << endl;
        if (occluded_bool){
            world2.cells[index].occluded = true;
            added_occ_count ++;
            cout << "new fake occlus/ion ...................."<< world2.cells[index].coordinates << endl;
        }
    }
    //cout << cg2[map2.find(Coordinates(-11,1))] << endl;
    cout << "occlusion count" << added_occ_count << endl;


    //cout << world_new << endl;
    //world_new.cells[0].occluded = 1;
    //cout << world2 << endl;
    cout << map2.cells.occluded_cells();
    Graph graph = world2.create_graph();
//    int i[] =  {true, true, false};
//
//    int sum = accumulate(i, i+2, 0);
//    cout << "sum " << sum << endl;


    // occlude cells
    //cout << world.cells[0].occluded << endl;
    //cout << world.cells


//    cout << map.cells.free_cells() << endl;
//    cout << map.cells.occluded_cells() << endl;
//    //cout << map.cells. << endl;
//    cout << wc.cell_coordinates << endl;
//    cout << wi.cell_locations << endl;

    //m[].occluded;

    //Cell_group free_cells = g.cells.free_cells();


    // create new paths
    Paths paths(graph);
    //auto new_path = paths.get_astar(graph);
    //cout << new_path << endl;

    //new_path.save("new_file_test");
    cout << "done" << endl;
    //cout = new_path;

    //Paths Paths::get_astar(world.create_graph());


    //auto pb = Resources::from("paths").key("hexagonal").key(occlusions_name).key("astar").get_resource<Path_builder>();

    // save paths




    return 0;
}