
# include <Siv3D.hpp> // OpenSiv3D v0.6.3

struct AfinParameter3D {
	double a;
	double b;
	double c;
	double d;
	double e;
	double f;
	double g;
	double h;
	double i;
	double j;
	double k;
	double l;
	double m;
	double n;
	double o;
	double p;
};
struct Angle {
	double w;
	double h;
};

struct _Vec3 {
	double x;
	double y;
	double z;
};
struct Object {
	Angle angle;
	_Vec3 pos;
};
struct _Triangle2D {
	Vec2 p0;
	Vec2 p1;
	Vec2 p2;
};
struct _Triangle3D {
	_Vec3 p0;
	_Vec3 p1;
	_Vec3 p2;
};

struct _Polygon3D {
	_Triangle3D points;
	Color color;
};
struct _Polygon {
	_Triangle2D points;
	Color color;
};
struct _Model {
	Array<_Polygon3D> shape;
	Object object;
	_Vec3 zahyo; //ゲームのフィールド格子)上の座標
	int hp;
};
struct _distTable {
	int id;
	double dist;
};
struct star {//一番後ろのレイヤーで左から右に動かす
	Vec2 pos;
	double size;
	double speed;
};
AfinParameter3D viewingPiperine;
const double CELL_PER = 5;
const int SIDE_CELLS = 15;
const int MARGIN = 5;
const int CELL_SIZE = 6;
const int MAX_HP = 200;
const int HISTORY_SIZE = 1000;
// 共通
_Vec3 changePos3D(_Vec3 p, AfinParameter3D afin) {//点の座標変換
	_Vec3 res;
	res.x = afin.a * p.x + afin.b * p.y + afin.c * p.z + afin.d;
	res.y = afin.e * p.x + afin.f * p.y + afin.g * p.z + afin.h;
	res.z = afin.i * p.x + afin.j * p.y + afin.k * p.z + afin.l;
	return res;
}
_Polygon3D transFormTriangle3D(_Polygon3D t, AfinParameter3D afin) {//三角形の座標変換
	t.points.p0 = changePos3D(t.points.p0, afin);
	t.points.p1 = changePos3D(t.points.p1, afin);
	t.points.p2 = changePos3D(t.points.p2, afin);
	return t;
}
Array<_Polygon3D> transFormModel(Array<_Polygon3D> triangles, AfinParameter3D afin) {
	return triangles.map([afin](_Polygon3D t) { return transFormTriangle3D(t, afin); });
}
Array<_Model> transFormModels(Array<_Model> models, AfinParameter3D afin) {//立体の座標変換
	for (int i = 0; i < models.size(); i++) {
		models[i].shape = transFormModel(models[i].shape, afin);
	}
	return models;
}

AfinParameter3D combineAfin(AfinParameter3D x, AfinParameter3D y) {//変換式を組み合わせる
	AfinParameter3D res;
	res.a = x.a * y.a + x.e * y.b + x.i * y.c + x.m * y.d;
	res.b = x.b * y.a + x.f * y.b + x.j * y.c + x.n * y.d;
	res.c = x.c * y.a + x.g * y.b + x.k * y.c + x.o * y.d;
	res.d = x.d * y.a + x.h * y.b + x.l * y.c + x.p * y.d;
	res.e = x.a * y.e + x.e * y.f + x.i * y.g + x.m * y.h;
	res.f = x.b * y.e + x.f * y.f + x.j * y.g + x.n * y.h;
	res.g = x.c * y.e + x.g * y.f + x.k * y.g + x.o * y.h;
	res.h = x.d * y.e + x.h * y.f + x.l * y.g + x.p * y.h;
	res.i = x.a * y.i + x.e * y.j + x.i * y.k + x.m * y.l;
	res.j = x.b * y.i + x.f * y.j + x.j * y.k + x.n * y.l;
	res.k = x.c * y.i + x.g * y.j + x.k * y.k + x.o * y.l;
	res.l = x.d * y.i + x.h * y.j + x.l * y.k + x.p * y.l;
	res.m = x.a * y.m + x.e * y.n + x.i * y.o + x.m * y.p;
	res.n = x.b * y.m + x.f * y.n + x.j * y.o + x.n * y.p;
	res.o = x.c * y.m + x.g * y.n + x.k * y.o + x.o * y.p;
	res.p = x.d * y.m + x.h * y.n + x.l * y.o + x.p * y.p;
	return res;
}

//-----
//ポリゴンの表裏判定
//-------
//ベクトル外積
_Vec3 cross_product(const _Vec3 vl, const _Vec3 vr)
{
	_Vec3 ret;
	ret.x = vl.y * vr.z - vl.z * vr.y;
	ret.y = vl.z * vr.x - vl.x * vr.z;
	ret.z = vl.x * vr.y - vl.y * vr.x;

	return ret;
}

//ベクトル内積
double dot_product(const _Vec3 vl, const _Vec3 vr) {
	return vl.x * vr.x + vl.y * vr.y + vl.z * vr.z;
}

// ベクトルvに対してポリゴンが表裏どちらを向くかを求める
// 戻り値    1:表    0:裏   
int polygon_side_chk(_Triangle3D t, _Vec3 v) {

	//ABCが三角形かどうか。ベクトルvが0でないかの判定は省略します
	_Vec3 A = t.p0;
	_Vec3 B = t.p1;
	_Vec3 C = t.p2;
	//AB BCベクトル
	_Vec3 AB;
	_Vec3 BC;

	AB.x = B.x - A.x;
	AB.y = B.y - A.y;
	AB.z = B.z - A.z;

	BC.x = C.x - A.x;
	BC.y = C.y - A.y;
	BC.z = C.z - A.z;

	//AB BCの外積
	_Vec3 c = cross_product(AB, BC);
	double dist = t.p0.z + t.p1.z + t.p2.z;
	if (dist < 3) {
		return 0;
	}
	//ベクトルvと内積。順、逆方向かどうか調べる
	double d = dot_product(v, c);

	if (d < 0.0) {
		return 1;    //ポリゴンはベクトルvから見て表側
	}
	return 0;
}
//三角形の中心のz座標を比較
int isFartherModel(const void* t, const void* a) {
	double targetDist = ((_distTable*)t)->dist;
	double dist = ((_distTable*)a)->dist;
	return 	targetDist > dist ? 1 : -1;
}
bool isFartherModel2(_Vec3 t, _Vec3 a) {
	return t.z > a.z;
}

//三角形を中心のz座標を基準に大きい順で並び替え
Array<_Model> sortModel(Array<_Model> models) {//奥行ソート
	_distTable* heap;
	heap = (_distTable*)malloc(sizeof(_distTable) * models.size());//メモリの確保

	for (int i = 0; i < models.size(); i++) {
		heap[i].id = i;
		heap[i].dist = models[i].shape[0].points.p0.z;
	}
	_distTable a[3] = { {1,200},{1,300},{1,100} };
	qsort(a, 3, sizeof(a[0]), isFartherModel);
	qsort(heap, models.size(), sizeof(heap[0]), isFartherModel);

	Array<_Model> res;


	for (int i = 0; i < models.size(); i++) {//デバッグ
		double dist = models[heap[i].id].shape[0].points.p0.z;
		//Rect(0, i * 2, dist / 2, 2).draw();
		res << models[heap[i].id];
	}
	free(heap);
	return res;
}
Array<_Model> sortModel2(Array<_Model> models) {//奥行ソート

	for (int i = 0; i < models.size(); i++) {
		for (int j = i; j < models.size(); j++) {
			if (isFartherModel2(models[j].shape[0].points.p0, models[i].shape[0].points.p0)) {
				_Model tmp = models[i];
				models[i] = models[j];
				models[j] = tmp;
			}
		}
	}


	return models;
}
// 投影変換　3次元空間上の点を2次元に配置
Vec2 toVec2(_Vec3 pos) {
	return Vec2{ pos.x * 1.3,pos.y * 1.3 };//平行投影z座標を無視
	//return Vec2{ pos.x/pos.z*200,pos.y/pos.z*200 };//投視投影　現時点だと歪んで見える
}
// 3dの三角形を2dに変換
_Polygon renderTriangle(_Polygon3D t) {
	_Polygon result;
	result.points.p0 = toVec2(t.points.p0);
	result.points.p1 = toVec2(t.points.p1);
	result.points.p2 = toVec2(t.points.p2);
	result.color = t.color;

	return result;
}
// 立体を2dに変換
Array<_Polygon> renderModel(Array<_Polygon3D> triangles) {
	_Polygon n = {};

	return triangles.map([n](_Polygon3D t) { return polygon_side_chk(t.points, _Vec3{ 0,0,1 }) ? renderTriangle(t) : n; });
}
//　複数の立体を2dに変換
Array<_Polygon> render(Array<_Model> models) {
	Array<_Polygon> res = {};
	models = sortModel(models);
	for (int i = 0; i < models.size(); i++) {
		Array<_Polygon> toAdd = renderModel(models[i].shape);
		for (int j = 0; j < toAdd.size(); j++) {
			res << toAdd[j];
		}
	}
	return res;
}
//ビューポート変換(画面に収める範囲の調整)
Vec2 moveCenterPos(Vec2 p) {
	return p + Scene::Center();
}
_Polygon moveCenterTriangle(_Polygon t) {
	t.points.p0 = moveCenterPos(t.points.p0);
	t.points.p1 = moveCenterPos(t.points.p1);
	t.points.p2 = moveCenterPos(t.points.p2);
	return t;
}
Array<_Polygon> moveCenterModel(Array<_Polygon> triangles) {
	return triangles.map([](_Polygon t) { return moveCenterTriangle(t); });
}

//モデリング変換　立体をその向きや座標に応じて３次元空間上に配置
Array<_Polygon3D> toWorldModel(Array<_Polygon3D> triangles, Object object) {
	AfinParameter3D afin1, afin2, afin3;
	double w = object.angle.w / 50;
	double h = object.angle.h / 50;
	afin1 = { cos(w),0,-sin(w),0,0,1,0,0,sin(w),0,cos(w),0 };
	afin2 = { 1,0,0,0,
		0,cos(h),sin(h),0,
		0,-sin(h),cos(h),0 };
	triangles = transFormModel(triangles, combineAfin(afin2, afin1));
	viewingPiperine = combineAfin(afin2, afin1);
	_Vec3 p = object.pos;
	afin3 = { 1,0,0,p.x,0,1,0,p.y,0,0,1,p.z };
	triangles = transFormModel(triangles, afin3);
	return triangles;
}
Array<_Model> toWorld(Array<_Model> models) {
	for (int i = 0; i < models.size(); i++) {
		models[i].shape = toWorldModel(models[i].shape, models[i].object);
	}
	return models;
}
//カメラが原点、z軸正の方向を向くように立体を移動
Array<_Polygon3D> conversionFieldModel(Array<_Polygon3D> triangles, Object camera) {
	AfinParameter3D afin1, afin2, afin3;

	_Vec3 p = camera.pos;
	afin3 = { 1,0,0,-p.x,0,1,0,-p.y,0,0,1,-p.z };
	triangles = transFormModel(triangles, afin3);
	double w = camera.angle.w / 50;
	double h = camera.angle.h / 50;
	afin1 = { cos(w),0,-sin(w),0,0,1,0,0,sin(w),0,cos(w),0 };
	afin2 = { 1,0,0,0,0,cos(h),sin(h),0,0,-sin(h),cos(h),0 };
	triangles = transFormModel(triangles, combineAfin(afin2, afin1));

	return triangles;
}
Array<_Model> conversionField(Array<_Model> models, Object camera) {
	for (int i = 0; i < models.size(); i++) {
		models[i].shape = toWorldModel(models[i].shape, camera);
	}
	return models;
}

//立体の大きさをrate倍する
Array<_Polygon3D> resizeModel(Array<_Polygon3D> model, double rate) {
	AfinParameter3D afin = { rate,0,0,0,0,rate,0,0,0,0,rate,0,0,0,0,1 };
	return transFormModel(model, afin);
}
//立体の色を色cに統一する
Array<_Polygon3D> paintModel(Array<_Polygon3D> model, Color c) {
	for (int i = 0; i < model.size(); i++) {
		model[i].color = c;
	}
	return model;
}
// posで指定した位置に立体を配置
Array<_Polygon3D> putModel(Array<_Polygon3D> models, _Vec3 pos) {
	AfinParameter3D afin = { 1,0,0,pos.x,0,1,0,pos.y,0,0,1,pos.z,0,0,0,1 };
	return transFormModel(models, afin);
}
// 初期配置をランダムで取得
Grid<int32> addField(Grid<int32> fieldState) {
	int mx = rand() % MARGIN;
	int my = rand() % MARGIN;
	int mz = rand() % MARGIN;
	for (int i = MARGIN - mx; i < SIDE_CELLS - mx; i++) {
		for (int j = MARGIN - my; j < SIDE_CELLS - my; j++) {
			for (int k = MARGIN - mz; k < SIDE_CELLS - mz; k++) {
				if (rand() % 10000 <= CELL_PER * 100) {
					fieldState[i][j] |= 1 << k;
				}
			}
		}
	}
	return fieldState;
}
Grid<int32> getVoidField() {
	Grid<int32> fieldState(SIDE_CELLS, SIDE_CELLS, 0);
	return fieldState;
}
// フィールドの範囲内か判定
bool isInField(_Vec3 p) {
	bool check = true;
	if (p.x < 0 || p.x >= SIDE_CELLS)
		check = false;
	if (p.y < 0 || p.y >= SIDE_CELLS)
		check = false;
	if (p.z < 0 || p.z >= SIDE_CELLS)
		check = false;
	return check;
}

// 周囲のセルに応じたセルの状態を取得
bool getCellState(_Vec3 pos, Grid<int> field) {//指定したブロックの値を取得
	_Vec3 p[2];
	double score = 0;
	bool isAlive = false;
	int state;
	int count = 0;
	for (int i = 0; i <= 1; i++) {
		for (int j = 0; j <= 1; j++) {
			for (int k = 0; k <= 1; k++) {
				if (i == 0 && j == 0 && k == 0)continue;
				p[0] = _Vec3{ i + pos.x,j + pos.y,k + pos.z };
				p[1] = _Vec3{ -i + pos.x,-j + pos.y,-k + pos.z };
				state = 0;
				for (int i = 0; i < 2; i++) {
					if (isInField(p[i])) {
						isAlive = field[int(p[i].x)][int(p[i].y)] >> int(p[i].z) & 1;
						state |= isAlive << i;
						if (i * j * k == 0 && isAlive) {
							score++;
						}
						else if (isAlive) {
							score += 0.5;
						}
					}
				}
				if (state == 0x11) count++;
			}
		}
	}

	bool res = false;
	if (field[int(pos.x)][int(pos.y)] >> int(pos.z) & 1) {
		if (score >= 4 && score <= 6)res = true;
	}
	else {
		if (score >= 3 && score <= 5)res = true;
	}
	if (count > 0)res = false;
	return res;
}

// 次のフィールドの状態を取得
Grid<int32> getNextField(Grid<int32> current) {
	Grid<int32> next(SIDE_CELLS, SIDE_CELLS, 0);
	bool isAlive;
	for (int i = 0; i < SIDE_CELLS; i++) {
		for (int j = 0; j < SIDE_CELLS; j++) {
			for (int k = 0; k < SIDE_CELLS; k++) {
				isAlive = getCellState(_Vec3{ double(i),double(j),double(k) }, current);
				if (isAlive) {
					next[i][j] |= 1 << k;//1に書き換え
				}
			}
		}
	}
	return next;
}

// フィールドの状態をもとに立体を取得
Array<_Model> fieldToModels(Grid<int32> field, Array<_Model> current, Array<_Polygon3D> cubePolygons, Object core) {
	Array<_Polygon3D> framePolygons = resizeModel(cubePolygons, SIDE_CELLS + 1);
	framePolygons = paintModel(framePolygons, { 0,255,0,0 });
	Array<_Model> models = {
		{framePolygons,core,{0,0,0},-1},//仮のバグ対策
		{framePolygons,core,{0,0,0},-1},
		{framePolygons,core,{0,0,0},-1},
		{framePolygons,core,{0,0,0},-1},
		{framePolygons,core,{0,0,0},-1},
		{framePolygons,core,{0,0,0},-1},
	};
	_Vec3 p = {};

	for (int i = 0; i < current.size(); i++) {
		p = current[i].zahyo;
		if (field[int(p.x)][int(p.y)] >> int(p.z) & 1) {
			field[int(p.x)][int(p.y)] ^= 1 << int(p.z);//1->0に書き換え
			models << current[i];
		}
	}
	for (int i = 0; i < SIDE_CELLS; i++) {
		p.x = CELL_SIZE * 2 * (i - SIDE_CELLS / 2);
		for (int j = 0; j < SIDE_CELLS; j++) {
			p.y = CELL_SIZE * 2 * (j - SIDE_CELLS / 2);
			for (int k = 0; k < SIDE_CELLS; k++) {
				p.z = CELL_SIZE * 2 * (k - SIDE_CELLS / 2);
				if (field[i][j] >> k & 1) {
					models << _Model{ putModel(cubePolygons,p), core, { double(i),double(j),double(k) }, MAX_HP };
				}
			}
		}
	}
	return models;
}
//指定座標の中心からの距離を取得
double getDistToCore(_Vec3 p) {
	double x = p.x - SIDE_CELLS / 2;
	double y = p.y - SIDE_CELLS / 2;
	double z = p.z - SIDE_CELLS / 2;
	return x * x + y * y + z * z;
}

//　中心からの距離に応じて色を変える
Array<_Model> coloringModels(Array<_Model> models) {
	Array<_Model> res = {
		models[0]
	};
	for (int i = 0; i < models.size(); i++) {
		double hue = getDistToCore(models[i].zahyo);
		if (models[i].hp > 0)
			models[i].shape = paintModel(models[i].shape, HSV{ hue * 3,0.6,1.0 });
	}
	return models;
}
int incrementInRing(int i, int add, int size) {
	return (i + add) % size;
}
void drawGraph(Array<int> history, int now) {
	int i = (now + HISTORY_SIZE - Scene::Width()) % HISTORY_SIZE;
	int next = i;
	int count = 0;
	while (count < Scene::Width() - 1) {
		Line(count, Scene::Height() - history[i] / 4, count + 1, Scene::Height() - history[next] / 4).draw(2);
		i++;
		i %= HISTORY_SIZE;
		next = incrementInRing(i, 1, HISTORY_SIZE);
		count++;
	}
}
void Main()
{
	srand(time(NULL));
	// 背景を黒にする
	Scene::SetBackground(Palette::Black);

	// 大きさ 60 のフォントを用意
	const Font font(60);

	//モデリング
	Array<_Vec3> samplePoints = {
		//{0,0,0},{200,0,0},{200,200,0},{0,200,0},
		{-20,20,-20},{0,200,0},{200,0,0},
		{0,0,200},{300,300,300},{0,0,100},{0,100,0},
		{100,80,100},{0,80,100}
	};
	Array<_Polygon3D> samplePolygons = {
		{_Triangle3D{ samplePoints[0], samplePoints[2], samplePoints[1] },Color{255,0,0}},
		{_Triangle3D{ samplePoints[0], samplePoints[1], samplePoints[3] },Color{100,255,0}},
		{_Triangle3D{ samplePoints[0], samplePoints[3], samplePoints[2] },Color{100,0,0}},
		{_Triangle3D{ samplePoints[4], samplePoints[1], samplePoints[2] },Color{100,0,255}},
		{_Triangle3D{ samplePoints[4], samplePoints[3], samplePoints[1] },Color{0,0,100}},
		{_Triangle3D{ samplePoints[4], samplePoints[2], samplePoints[3] },Color{100,100,100}},
	};
	Array<_Vec3> cubePoints = {
	{-CELL_SIZE,-CELL_SIZE,-CELL_SIZE},{CELL_SIZE,-CELL_SIZE,-CELL_SIZE},{CELL_SIZE,-CELL_SIZE,CELL_SIZE},{-CELL_SIZE,-CELL_SIZE,CELL_SIZE},
	{-CELL_SIZE,CELL_SIZE,-CELL_SIZE},{CELL_SIZE,CELL_SIZE,-CELL_SIZE},{CELL_SIZE,CELL_SIZE,CELL_SIZE},{-CELL_SIZE,CELL_SIZE,CELL_SIZE}
	};
	Array<_Polygon3D> cubePolygons = {
	{_Triangle3D{ cubePoints[0], cubePoints[3], cubePoints[1] },Color{0,255,0}},
	{_Triangle3D{ cubePoints[1], cubePoints[3], cubePoints[2] },Color{0,255,0}},
	{_Triangle3D{ cubePoints[4], cubePoints[5], cubePoints[7] },Color{255,0,0}},
	{_Triangle3D{ cubePoints[5], cubePoints[6], cubePoints[7] },Color{255,0,0}},
	{_Triangle3D{ cubePoints[0], cubePoints[5], cubePoints[4] },Color{0,0,255}},
	{_Triangle3D{ cubePoints[1], cubePoints[5], cubePoints[0] },Color{0,0,255}},
	{_Triangle3D{ cubePoints[0], cubePoints[4], cubePoints[7] },Color{0,255,255}},
	{_Triangle3D{ cubePoints[3], cubePoints[0], cubePoints[7] },Color{0,255,255}},

	{_Triangle3D{ cubePoints[2], cubePoints[7], cubePoints[6] },Color{255,255,0}},
	{_Triangle3D{ cubePoints[3], cubePoints[7], cubePoints[2] },Color{255,255,0}},

	{_Triangle3D{ cubePoints[2], cubePoints[6], cubePoints[5] },Color{255,0,255}},
	{_Triangle3D{ cubePoints[1], cubePoints[2], cubePoints[5] },Color{255,0,255}},

	};
	Object ex0 = { Angle{0,0},_Vec3{0,0,0} };
	Object core = { Angle{0,0},_Vec3{0,0,500} };
	Object ex2 = { Angle{0,-12},_Vec3{0,-40,500} };
	Object ex3 = { Angle{0,-12},_Vec3{50,50,50} };

	Grid<int32> fieldState(SIDE_CELLS, SIDE_CELLS, 0);//３次元配列　z軸は2進数で管理
	fieldState = addField(fieldState);
	Array<_Model> models = {};
	models = fieldToModels(fieldState, models, cubePolygons, core);
	models = coloringModels(models);
	//モデリング変換
	Array<_Model> models_W = toWorld(models);
	Object camera = { Angle{0,10},_Vec3{0,-100,0} };
	int time = 0;
	Array<int> history(HISTORY_SIZE, 0);
	int now = 0;
	Vec2 center = Scene::Center();
	Vec2 Button1 = { center.x + 270, center.y + 180 };
	Vec2 Button2 = { center.x - 270, center.y + 180 };
	const RectF addButton(Arg::center(Button1), 80,80);
	const RectF resetButton(Arg::center(Button2), 80);

	while (System::Update())
	{
		const double delta = 200 * Scene::DeltaTime();

		// 上下左右キーで移動
		if (KeyA.pressed())
		{
			models[1].object.pos.x += delta;
		}

		if (KeyD.pressed())
		{
			models[1].object.pos.x -= delta;
		}

		if (KeyW.pressed())
		{
			models[1].object.pos.z -= delta;
		}

		if (KeyS.pressed())
		{
			models[1].object.pos.z += delta;
		}
		if (KeySpace.pressed())
		{
			models[1].object.pos.y += delta;
		}

		if (KeyShift.pressed())
		{
			models[1].object.pos.y -= delta;
		}
		if (addButton.leftClicked())
		{
			fieldState = addField(fieldState);
			models = fieldToModels(fieldState, models, cubePolygons, core);
		}
		if (resetButton.leftClicked())
		{
			fieldState = getVoidField();
			fieldState = addField(fieldState);
			models = fieldToModels(fieldState, models, cubePolygons, core);
		}
		if (time % 10 == 0) {
			fieldState = getNextField(fieldState);
			models = fieldToModels(fieldState, models, cubePolygons, models[0].object);
		}
		models = coloringModels(models);

		//ClearPrint();
		for (int i = 0; i < models.size(); i++) {
			models[i].object.angle.w += delta / 3;
		}

		//視点移動
		//camera.angle.w = Cursor::Pos().x - Scene::Center().x;
		//camera.angle.h = Cursor::Pos().y - Scene::Center().y;

		//モデリング変換
		models_W = toWorld(models);

		//視野変換
		Array<_Model> models_W_camera = conversionField(models_W, camera);

		// 投影変換
		Array<_Polygon> t = render(models_W_camera);

		// ビューポート変換
		t = moveCenterModel(t);
		//描画
		t.map([](_Polygon t) {
			Triangle _t = { t.points.p0,t.points.p1,t.points.p2 };
			_t.draw(t.color);  return 0;
			});

		//履歴
		if (time % 10) {
			history[now] = models.size();
			now++;
			now %= HISTORY_SIZE;
		}
		RectF(Arg::center(Button1), 80).draw(Color(2, 210, 22));
		RectF(Arg::center(Button1), 60).draw(Color(10, 244, 32));
		addButton.draw(addButton.mouseOver() ? Color(0, 0, 0, 0) : Color(0, 0, 0, 40));
		
		RectF(Arg::center(Button2), 80).draw(Color(242, 10, 32));
		RectF(Arg::center(Button2), 60).draw(Color(245, 88, 52));
		resetButton.draw(resetButton.mouseOver() ? Color(0, 0, 0, 0) : Color(0, 0, 0, 40));
		drawGraph(history, now);

		time++;
		//デバッグ
		//Print << Cursor::Pos(); // 現在のマウスカーソル座標を表示
		//Print << camera.angle.w;
	}
}

//参考　http://www.sousakuba.com/Programming/gs_polygon_inside_outside.html ポリゴン表裏判定

//
// = アドバイス =
// Debug ビルドではプログラムの最適化がオフになります。
// 実行速度が遅いと感じた場合は Release ビルドを試しましょう。
// アプリをリリースするときにも、Release ビルドにするのを忘れないように！
//
// 思ったように動作しない場合は「デバッグの開始」でプログラムを実行すると、
// 出力ウィンドウに詳細なログが表示されるので、エラーの原因を見つけやすくなります。
//
// = お役立ちリンク =
//
// OpenSiv3D リファレンス
// https://siv3d.github.io/ja-jp/
//
// チュートリアル
// https://siv3d.github.io/ja-jp/tutorial/basic/
//
// よくある間違い
// https://siv3d.github.io/ja-jp/articles/mistakes/
//
// サポートについて
// https://siv3d.github.io/ja-jp/support/support/
//
// Siv3D ユーザコミュニティ Slack への参加
// https://siv3d.github.io/ja-jp/community/community/
//
// 新機能の提案やバグの報告
// https://github.com/Siv3D/OpenSiv3D/issues
//
